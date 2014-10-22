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

#include "TComMotionInfo.h"
#include "common.h"

using namespace x265;

void TComCUMvField::initialize(const MVFieldMemPool& p, uint32_t parts, int cuInstance, int list)
{
    mv     = p.mvMemBlock     + (cuInstance * 2 + list) * parts;
    mvd    = p.mvdMemBlock    + (cuInstance * 2 + list) * parts;
    refIdx = p.refIdxMemBlock + (cuInstance * 2 + list) * parts;
    numPartitions = parts;
}

void TComCUMvField::copyFrom(const TComCUMvField& src, int numPartSrc, int partAddrDst)
{
    int sizeInMv = sizeof(MV) * numPartSrc;

    memcpy(mv     + partAddrDst, src.mv,     sizeInMv);
    memcpy(mvd    + partAddrDst, src.mvd,    sizeInMv);
    memcpy(refIdx + partAddrDst, src.refIdx, sizeof(*refIdx) * numPartSrc);
}

void TComCUMvField::copyTo(TComCUMvField& dst, int partAddrDst) const
{
    copyTo(dst, partAddrDst, 0, numPartitions);
}

void TComCUMvField::copyTo(TComCUMvField& dst, int partAddrDst, uint32_t offset, uint32_t numPart) const
{
    int sizeInMv = sizeof(MV) * numPart;
    int partOffset = offset + partAddrDst;

    memcpy(dst.mv     + partOffset, mv     + offset, sizeInMv);
    memcpy(dst.mvd    + partOffset, mvd    + offset, sizeInMv);
    memcpy(dst.refIdx + partOffset, refIdx + offset, sizeof(*refIdx) * numPart);
}

template<typename T>
void TComCUMvField::setAll(T *p, T const & val, PartSize cuMode, int absPartIdx, int puIdx)
{
    int i;

    p += absPartIdx;
    int numElements = numPartitions;

    switch (cuMode)
    {
    case SIZE_2Nx2N:
        for (i = 0; i < numElements; i++)
            p[i] = val;
        break;

    case SIZE_2NxN:
        numElements >>= 1;
        for (i = 0; i < numElements; i++)
            p[i] = val;
        break;

    case SIZE_Nx2N:
        numElements >>= 2;
        for (i = 0; i < numElements; i++)
        {
            p[i] = val;
            p[i + 2 * numElements] = val;
        }
        break;

    case SIZE_2NxnU:
    {
        int curPartNumQ = numElements >> 2;
        if (!puIdx)
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
                pT[i] = val;

            pT = p + curPartNumQ;
            for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
                pT[i] = val;
        }
        break;
    }

    case SIZE_2NxnD:
    {
        int curPartNumQ = numElements >> 2;
        if (!puIdx)
        {
            T *pT  = p;
            for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
                pT[i] = val;

            pT = p + (numElements - curPartNumQ);
            for (i = 0; i < (curPartNumQ >> 1); i++)
                pT[i] = val;
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
        if (!puIdx)
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
        if (!puIdx)
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

    case SIZE_NxN:
    default:
        X265_CHECK(0, "unknown partition type\n");
        break;
    }
}

void TComCUMvField::setAllMv(const MV& inmv, PartSize cuMode, int absPartIdx, int puIdx)
{
    setAll(mv, inmv, cuMode, absPartIdx, puIdx);
}

void TComCUMvField::setAllRefIdx(int inRefIdx, PartSize cuMode, int absPartIdx, int puIdx)
{
    setAll(refIdx, static_cast<char>(inRefIdx), cuMode, absPartIdx, puIdx);
}
