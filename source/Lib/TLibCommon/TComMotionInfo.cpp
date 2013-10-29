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

/** \file     TComMotionInfo.cpp
    \brief    motion information handling classes
*/

#include <memory.h>
#include "TComMotionInfo.h"
#include "assert.h"
#include <cstdlib>

using namespace x265;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// Create / destroy
// --------------------------------------------------------------------------------------------------------------------

void TComCUMvField::create(uint32_t numPartition)
{
    assert(m_mv     == NULL);
    assert(m_mvd    == NULL);
    assert(m_refIdx == NULL);

    m_mv     = new MV[numPartition];
    m_mvd    = new MV[numPartition];
    m_refIdx = new char[numPartition];

    m_numPartitions = numPartition;
}

void TComCUMvField::destroy()
{
    assert(m_mv     != NULL);
    assert(m_mvd    != NULL);
    assert(m_refIdx != NULL);

    delete[] m_mv;
    delete[] m_mvd;
    delete[] m_refIdx;

    m_mv     = NULL;
    m_mvd    = NULL;
    m_refIdx = NULL;

    m_numPartitions = 0;
}

// --------------------------------------------------------------------------------------------------------------------
// Clear / copy
// --------------------------------------------------------------------------------------------------------------------

void TComCUMvField::clearMvField()
{
    for (int i = 0; i < m_numPartitions; i++)
    {
        m_mv[i] = 0;
        m_mvd[i] = 0;
    }

    assert(sizeof(*m_refIdx) == 1);
    memset(m_refIdx, NOT_VALID, m_numPartitions * sizeof(*m_refIdx));
}

void TComCUMvField::copyFrom(TComCUMvField const * cuMvFieldSrc, int numPartSrc, int partAddrDst)
{
    int sizeInMv = sizeof(MV) * numPartSrc;

    memcpy(m_mv     + partAddrDst, cuMvFieldSrc->m_mv,     sizeInMv);
    memcpy(m_mvd    + partAddrDst, cuMvFieldSrc->m_mvd,    sizeInMv);
    memcpy(m_refIdx + partAddrDst, cuMvFieldSrc->m_refIdx, sizeof(*m_refIdx) * numPartSrc);
}

void TComCUMvField::copyTo(TComCUMvField* cuMvFieldDst, int partAddrDst) const
{
    copyTo(cuMvFieldDst, partAddrDst, 0, m_numPartitions);
}

void TComCUMvField::copyTo(TComCUMvField* cuMvFieldDst, int partAddrDst, uint32_t offset, uint32_t numPart) const
{
    int sizeInMv = sizeof(MV) * numPart;
    int partOffset = offset + partAddrDst;

    memcpy(cuMvFieldDst->m_mv     + partOffset, m_mv     + offset, sizeInMv);
    memcpy(cuMvFieldDst->m_mvd    + partOffset, m_mvd    + offset, sizeInMv);
    memcpy(cuMvFieldDst->m_refIdx + partOffset, m_refIdx + offset, sizeof(*m_refIdx) * numPart);
}

// --------------------------------------------------------------------------------------------------------------------
// Set
// --------------------------------------------------------------------------------------------------------------------

template<typename T>
void TComCUMvField::setAll(T *p, T const & val, PartSize cuMode, int partAddr, uint32_t depth, int partIdx)
{
    int i;

    p += partAddr;
    int numElements = m_numPartitions >> (2 * depth);

    switch (cuMode)
    {
    case SIZE_2Nx2N:
        for (i = 0; i < numElements; i++)
        {
            p[i] = val;
        }

        break;

    case SIZE_2NxN:
        numElements >>= 1;
        for (i = 0; i < numElements; i++)
        {
            p[i] = val;
        }

        break;

    case SIZE_Nx2N:
        numElements >>= 2;
        for (i = 0; i < numElements; i++)
        {
            p[i] = val;
            p[i + 2 * numElements] = val;
        }

        break;

    case SIZE_NxN:
        numElements >>= 2;
        for (i = 0; i < numElements; i++)
        {
            p[i] = val;
        }

        break;
    case SIZE_2NxnU:
    {
        int curPartNumQ = numElements >> 2;
        if (partIdx == 0)
        {
            T *pT  = p;
            T *pT2 = p + curPartNumQ;
            for (i = 0; i < (curPartNumQ >> 1); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }
        }
        else
        {
            T *pT  = p;
            for (i = 0; i < (curPartNumQ >> 1); i++)
            {
                pT[i] = val;
            }

            pT = p + curPartNumQ;
            for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
            {
                pT[i] = val;
            }
        }
        break;
    }
    case SIZE_2NxnD:
    {
        int curPartNumQ = numElements >> 2;
        if (partIdx == 0)
        {
            T *pT  = p;
            for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
            {
                pT[i] = val;
            }

            pT = p + (numElements - curPartNumQ);
            for (i = 0; i < (curPartNumQ >> 1); i++)
            {
                pT[i] = val;
            }
        }
        else
        {
            T *pT  = p;
            T *pT2 = p + curPartNumQ;
            for (i = 0; i < (curPartNumQ >> 1); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }
        }
        break;
    }
    case SIZE_nLx2N:
    {
        int curPartNumQ = numElements >> 2;
        if (partIdx == 0)
        {
            T *pT  = p;
            T *pT2 = p + (curPartNumQ << 1);
            T *pT3 = p + (curPartNumQ >> 1);
            T *pT4 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);

            for (i = 0; i < (curPartNumQ >> 2); i++)
            {
                pT[i] = val;
                pT2[i] = val;
                pT3[i] = val;
                pT4[i] = val;
            }
        }
        else
        {
            T *pT  = p;
            T *pT2 = p + (curPartNumQ << 1);
            for (i = 0; i < (curPartNumQ >> 2); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }

            pT  = p + (curPartNumQ >> 1);
            pT2 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);
            for (i = 0; i < ((curPartNumQ >> 2) + curPartNumQ); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }
        }
        break;
    }
    case SIZE_nRx2N:
    {
        int curPartNumQ = numElements >> 2;
        if (partIdx == 0)
        {
            T *pT  = p;
            T *pT2 = p + (curPartNumQ << 1);
            for (i = 0; i < ((curPartNumQ >> 2) + curPartNumQ); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }

            pT  = p + curPartNumQ + (curPartNumQ >> 1);
            pT2 = p + numElements - curPartNumQ + (curPartNumQ >> 1);
            for (i = 0; i < (curPartNumQ >> 2); i++)
            {
                pT[i] = val;
                pT2[i] = val;
            }
        }
        else
        {
            T *pT  = p;
            T *pT2 = p + (curPartNumQ >> 1);
            T *pT3 = p + (curPartNumQ << 1);
            T *pT4 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);
            for (i = 0; i < (curPartNumQ >> 2); i++)
            {
                pT[i] = val;
                pT2[i] = val;
                pT3[i] = val;
                pT4[i] = val;
            }
        }
        break;
    }
    default:
        assert(0);
        break;
    }
}

void TComCUMvField::setAllMv(const MV& mv, PartSize cuMode, int partAddr, uint32_t depth, int partIdx)
{
    setAll(m_mv, mv, cuMode, partAddr, depth, partIdx);
}

void TComCUMvField::setAllMvd(const MV& mvd, PartSize cuMode, int partAddr, uint32_t depth, int partIdx)
{
    setAll(m_mvd, mvd, cuMode, partAddr, depth, partIdx);
}

void TComCUMvField::setAllRefIdx(int refIdx, PartSize cuMode, int partAddr, uint32_t depth, int partIdx)
{
    setAll(m_refIdx, static_cast<char>(refIdx), cuMode, partAddr, depth, partIdx);
}

void TComCUMvField::setAllMvField(const TComMvField& mvField, PartSize cuMode, int partAddr, uint32_t depth, int partIdx)
{
    setAllMv(mvField.mv, cuMode, partAddr, depth, partIdx);
    setAllRefIdx(mvField.refIdx, cuMode, partAddr, depth, partIdx);
}

//! \}
