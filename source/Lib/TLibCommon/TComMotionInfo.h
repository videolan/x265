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
    \todo     TComMvField seems to be better to be inherited from x265::MV
*/

#ifndef __TCOMMOTIONINFO__
#define __TCOMMOTIONINFO__

#include <memory.h>
#include "CommonDef.h"
#include "mv.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Type definition
// ====================================================================================================================

/// parameters for AMVP
typedef struct _AMVPInfo
{
    x265::MV m_mvCand[AMVP_MAX_NUM_CANDS_MEM];  ///< array of motion vector predictor candidates
    Int      m_num;                             ///< number of motion vector predictor candidates
} AMVPInfo;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// class for motion vector with reference index
class TComMvField
{
public:

    x265::MV  mv;
    Int       refIdx;

    TComMvField() : refIdx(NOT_VALID) {}

    Void setMvField(const x265::MV & _mv, Int _refIdx)
    {
        mv     = _mv;
        refIdx = _refIdx;
    }
};

/// class for motion information in one CU
class TComCUMvField
{
private:

    x265::MV* m_mv;
    x265::MV* m_mvd;
    Char*     m_refIdx;
    UInt      m_numPartitions;
    AMVPInfo  m_cAMVPInfo;

    template<typename T>
    Void setAll(T *p, T const & val, PartSize cuMode, Int partAddr, UInt depth, Int partIdx);

public:

    TComCUMvField() : m_mv(NULL), m_mvd(NULL), m_refIdx(NULL), m_numPartitions(0) {}

    ~TComCUMvField() {}

    // ------------------------------------------------------------------------------------------------------------------
    // create / destroy
    // ------------------------------------------------------------------------------------------------------------------

    Void create(UInt numPartition);
    Void destroy();

    // ------------------------------------------------------------------------------------------------------------------
    // clear / copy
    // ------------------------------------------------------------------------------------------------------------------

    Void clearMvField();

    Void copyFrom(const TComCUMvField * cuMvFieldSrc, Int numPartSrc, Int partAddrDst);
    Void copyTo(TComCUMvField* cuMvFieldDst, Int partAddrDst) const;
    Void copyTo(TComCUMvField* cuMvFieldDst, Int partAddrDst, UInt offset, UInt numPart) const;

    // ------------------------------------------------------------------------------------------------------------------
    // get
    // ------------------------------------------------------------------------------------------------------------------

    const x265::MV & getMv(Int idx) const { return m_mv[idx]; }

    const x265::MV & getMvd(Int idx) const { return m_mvd[idx]; }

    Int getRefIdx(Int idx) const { return m_refIdx[idx]; }

    AMVPInfo* getAMVPInfo() { return &m_cAMVPInfo; }

    // ------------------------------------------------------------------------------------------------------------------
    // set
    // ------------------------------------------------------------------------------------------------------------------

    Void    setAllMv(const x265::MV& mv,              PartSize cuMode, Int partAddr, UInt depth, Int partIdx = 0);
    Void    setAllMvd(const x265::MV& mvd,            PartSize cuMode, Int partAddr, UInt depth, Int partIdx = 0);
    Void    setAllRefIdx(Int refIdx,                  PartSize mbMode, Int partAddr, UInt depth, Int partIdx = 0);
    Void    setAllMvField(const TComMvField& mvField, PartSize mbMode, Int partAddr, UInt depth, Int partIdx = 0);

    Void setNumPartition(Int numPart)
    {
        m_numPartitions = numPart;
    }

    Void linkToWithOffset(const TComCUMvField* src, Int offset)
    {
        m_mv     = src->m_mv     + offset;
        m_mvd    = src->m_mvd    + offset;
        m_refIdx = src->m_refIdx + offset;
    }

    Void compress(Char* pePredMode, Int scale);
};

//! \}

#endif // __TCOMMOTIONINFO__
