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

#ifndef __TCOMMOTIONINFO__
#define __TCOMMOTIONINFO__

#include <memory.h>
#include "CommonDef.h"
#include "mv.h"

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

// ====================================================================================================================
// Type definition
// ====================================================================================================================

/// parameters for AMVP
typedef struct _AMVPInfo
{
    MV m_mvCand[AMVP_MAX_NUM_CANDS_MEM];  ///< array of motion vector predictor candidates
    int      m_num;                             ///< number of motion vector predictor candidates
} AMVPInfo;

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
private:

    MV* m_mv;
    MV* m_mvd;
    char*     m_refIdx;
    UInt      m_numPartitions;
    AMVPInfo  m_cAMVPInfo;

    MV*       m_cmv_mv;
    char*     m_cmv_refIdx;

    template<typename T>
    void setAll(T *p, T const & val, PartSize cuMode, int partAddr, UInt depth, int partIdx);

public:

    TComCUMvField() : m_mv(NULL), m_mvd(NULL), m_refIdx(NULL), m_numPartitions(0), m_cmv_mv(NULL), m_cmv_refIdx(NULL) {}

    ~TComCUMvField() {}

    // ------------------------------------------------------------------------------------------------------------------
    // create / destroy
    // ------------------------------------------------------------------------------------------------------------------

    void create(UInt numPartition);
    void destroy();

    // ------------------------------------------------------------------------------------------------------------------
    // clear / copy
    // ------------------------------------------------------------------------------------------------------------------

    void clearMvField();

    void copyFrom(const TComCUMvField * cuMvFieldSrc, int numPartSrc, int partAddrDst);
    void copyTo(TComCUMvField* cuMvFieldDst, int partAddrDst) const;
    void copyTo(TComCUMvField* cuMvFieldDst, int partAddrDst, UInt offset, UInt numPart) const;

    // ------------------------------------------------------------------------------------------------------------------
    // get
    // ------------------------------------------------------------------------------------------------------------------

    const MV & getMv(int idx) const { return m_mv[idx]; }
    const MV & getMv_cmv(int idx) const { return m_cmv_mv[idx]; }

    const MV & getMvd(int idx) const { return m_mvd[idx]; }

    int getRefIdx(int idx) const { return m_refIdx[idx]; }
    int getRefIdx_cmv(int idx) const { return m_cmv_refIdx[idx]; }

    AMVPInfo* getAMVPInfo() { return &m_cAMVPInfo; }

    // ------------------------------------------------------------------------------------------------------------------
    // set
    // ------------------------------------------------------------------------------------------------------------------

    void    setAllMv(const MV& mv,              PartSize cuMode, int partAddr, UInt depth, int partIdx = 0);
    void    setAllMvd(const MV& mvd,            PartSize cuMode, int partAddr, UInt depth, int partIdx = 0);
    void    setAllRefIdx(int refIdx,                  PartSize mbMode, int partAddr, UInt depth, int partIdx = 0);
    void    setAllMvField(const TComMvField& mvField, PartSize mbMode, int partAddr, UInt depth, int partIdx = 0);

    void setNumPartition(int numPart)
    {
        m_numPartitions = numPart;
    }

    void linkToWithOffset(const TComCUMvField* src, int offset)
    {
        m_mv     = src->m_mv     + offset;
        m_mvd    = src->m_mvd    + offset;
        m_refIdx = src->m_refIdx + offset;
    }

    void compress(char* pePredMode, int scale);
};
}

//! \}

#endif // __TCOMMOTIONINFO__
