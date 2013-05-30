/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Chung Shin Yee <shinyee@multicorewareinc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#ifndef __WAVEFRONT__
#define __WAVEFRONT__

#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComPic.h"
#include "TLibEncoder/TEncCu.h"
#include "TLibEncoder/TEncSbac.h"

#include "threadpool.h"

namespace x265 {
// private x265 namespace

typedef struct CUComputeData
{
    CUComputeData(TComPic*          rpcPic,
                  TEncSbac****      ppppcRDSbacCoders,
                  TComSlice*        pcSlice,
                  TComBitCounter*   pcBitCounters,
                  TEncEntropy*      pcEntropyCoders,
                  TEncSbac*         pcSbacCoder,
                  TEncSbac*         pcBufferSbacCoders,
                  TEncSbac*         pcRDGoOnSbacCoders,
                  TComRdCost*       pcRdCost,
                  TEncCu*           pcCuEncoders,
                  TEncSearch*       pcPredSearches,
                  TComTrQuant*      pcTrQuants,
                  Bool              bWaveFrontsynchro)

        : pic(rpcPic),
          sbacCoders(ppppcRDSbacCoders),
          slice(pcSlice),
          bitCounters(pcBitCounters),
          entropyCoders(pcEntropyCoders),
          sbacCoder(pcSbacCoder),
          bufferSbacCoders(pcBufferSbacCoders),
          goOnSbacCoders(pcRDGoOnSbacCoders),
          rdCost(pcRdCost),
          cuEncoders(pcCuEncoders),
          predSearches(pcPredSearches),
          trQuants(pcTrQuants),
          waveFrontSynchro(bWaveFrontsynchro) {}

    TComPic* pic;
    TEncSbac**** sbacCoders;
    TComSlice* slice;
    TComBitCounter* bitCounters;

    TEncEntropy* entropyCoders;
    TEncSbac* sbacCoder;
    TEncSbac* bufferSbacCoders;
    TEncSbac* goOnSbacCoders;
    TComRdCost* rdCost;
    TEncCu* cuEncoders;
    TEncSearch* predSearches;
    TComTrQuant* trQuants;

    Bool waveFrontSynchro;
} CUComputeData;


struct CURowState
{
    CURowState() : active(false), curCol(0) {}

    void Initialize()
    {
        this->active = false;
        this->curCol = 0;

        // FIXME: How to initialize lock?
        // ...
    }

    Lock lock;
    volatile bool active;
    volatile int curCol;
};


class EncodingFrame : public QueueFrame
{
public:

    EncodingFrame(int nrows_, int ncols_, ThreadPool *pool);

    virtual ~EncodingFrame();

    void Initialize(CUComputeData* cdata);

    void Encode();

    void ProcessRow(int irow);

private:

    int nrows;
    int ncols;

    CUComputeData* data;
    CURowState* rows;
    Event complete;
};
}

#endif // ifndef __WAVEFRONT__
