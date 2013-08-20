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

/** \file     TComSampleAdaptiveOffset.h
    \brief    sample adaptive offset class (header)
*/

#ifndef __TCOMSAMPLEADAPTIVEOFFSET__
#define __TCOMSAMPLEADAPTIVEOFFSET__

#include "CommonDef.h"
#include "TComPic.h"

//! \ingroup TLibCommon
//! \{

#define NUM_DOWN_PART 4

enum SAOTypeLen
{
    SAO_EO_LEN    = 4,
    SAO_BO_LEN    = 4,
    SAO_MAX_BO_CLASSES = 32
};

enum SAOType
{
    SAO_EO_0 = 0,
    SAO_EO_1,
    SAO_EO_2,
    SAO_EO_3,
    SAO_BO,
    MAX_NUM_SAO_TYPE
};

typedef struct _SaoQTPart
{
    Int         bestType;
    Int         length;
    Int         subTypeIdx;                ///< indicates EO class or BO band position
    Int         offset[4];
    Int         startCUX;
    Int         startCUY;
    Int         endCUX;
    Int         endCUY;

    Int         partIdx;
    Int         partLevel;
    Int         partCol;
    Int         partRow;

    Int         downPartsIdx[NUM_DOWN_PART];
    Int         upPartIdx;

    Bool        bSplit;

    //---- encoder only start -----//
    Bool        bProcessed;
    Double      minCost;
    Int64       minDist;
    Int         minRate;
    //---- encoder only end -----//
} SAOQTPart;

typedef struct _SaoLcuParam
{
    Bool       mergeUpFlag;
    Bool       mergeLeftFlag;
    Int        typeIdx;
    Int        subTypeIdx;                ///< indicates EO class or BO band position
    Int        offset[4];
    Int        partIdx;
    Int        partIdxTmp;
    Int        length;
} SaoLcuParam;

struct SAOParam
{
    Bool       bSaoFlag[2];
    SAOQTPart* saoPart[3];
    Int        maxSplitLevel;
    Bool         oneUnitFlag[3];
    SaoLcuParam* saoLcuParam[3];
    Int          numCuInHeight;
    Int          numCuInWidth;
    ~SAOParam();
};

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define SAO_MAX_DEPTH                 4
#define SAO_BO_BITS                   5
#define LUMA_GROUP_NUM                (1 << SAO_BO_BITS)
#define MAX_NUM_SAO_OFFSETS           4
#define MAX_NUM_SAO_CLASS             33
// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// Sample Adaptive Offset class
class TComSampleAdaptiveOffset
{
protected:

    TComPic* m_pic;

    static const UInt m_maxDepth;
    static const Int m_numCulPartsLevel[5];
    static const UInt m_eoTable[9];
    static const Int m_numClass[MAX_NUM_SAO_TYPE];

    Int *m_offsetBo;
    Int *m_chromaOffsetBo;
    Int m_offsetEo[LUMA_GROUP_NUM];

    Int  m_picWidth;
    Int  m_picHeight;
    UInt m_maxSplitLevel;
    UInt m_maxCUWidth;
    UInt m_maxCUHeight;
    Int  m_numCuInWidth;
    Int  m_numCuInHeight;
    Int  m_numTotalParts;

    UInt m_saoBitIncreaseY;
    UInt m_saoBitIncreaseC; //for chroma
    UInt m_qp;

    Pel   *m_clipTable;
    Pel   *m_clipTableBase;
    Pel   *m_lumaTableBo;
    Pel   *m_chromaClipTable;
    Pel   *m_chromaClipTableBase;
    Pel   *m_chromaTableBo;
    Int   *m_upBuff1;
    Int   *m_upBuff2;
    Int   *m_upBufft;
    TComPicYuv* m_tmpYuv;  //!< temporary picture buffer pointer when non-across slice/tile boundary SAO is enabled

    Pel* m_tmpU1;
    Pel* m_tmpU2;
    Pel* m_tmpL1;
    Pel* m_tmpL2;
    Int     m_maxNumOffsetsPerPic;
    Bool    m_saoLcuBoundary;
    Bool    m_saoLcuBasedOptimization;

public:

    TComSampleAdaptiveOffset();
    virtual ~TComSampleAdaptiveOffset();

    Void create(UInt sourceWidth, UInt sourceHeight, UInt maxCUWidth, UInt maxCUHeight);
    Void destroy();

    Int  convertLevelRowCol2Idx(Int level, Int row, Int col) const;

    Void initSAOParam(SAOParam* saoParam, Int partLevel, Int partRow, Int partCol, Int parentPartIdx, Int startCUX, Int endCUX, Int startCUY, Int endCUY, Int yCbCr) const;
    Void allocSaoParam(SAOParam* saoParam) const;
    Void resetSAOParam(SAOParam* saoParam);
    static Void freeSaoParam(SAOParam* saoParam);

    Void SAOProcess(SAOParam* saoParam);
    Void processSaoCu(Int addr, Int saoType, Int yCbCr);
    Pel* getPicYuvAddr(TComPicYuv* picYuv, Int yCbCr, Int addr = 0);

    Void processSaoCuOrg(Int addr, Int partIdx, Int yCbCr); //!< LCU-basd SAO process without slice granularity
    Void createPicSaoInfo(TComPic* pic);
    Void destroyPicSaoInfo();

    Void resetLcuPart(SaoLcuParam* saoLcuParam);
    Void convertQT2SaoUnit(SAOParam* saoParam, UInt partIdx, Int yCbCr);
    Void convertOnePart2SaoUnit(SAOParam *saoParam, UInt partIdx, Int yCbCr);
    Void processSaoUnitAll(SaoLcuParam* saoLcuParam, Bool oneUnitFlag, Int yCbCr);
    Void setSaoLcuBoundary(int bVal)  { m_saoLcuBoundary = bVal != 0; }

    Bool getSaoLcuBoundary()           { return m_saoLcuBoundary; }

    Void setSaoLcuBasedOptimization(int bVal)  { m_saoLcuBasedOptimization = bVal != 0; }

    Bool getSaoLcuBasedOptimization()           { return m_saoLcuBasedOptimization; }

    Void resetSaoUnit(SaoLcuParam* saoUnit);
    Void copySaoUnit(SaoLcuParam* saoUnitDst, SaoLcuParam* saoUnitSrc);
};
Void PCMLFDisableProcess(TComPic* pic);

//! \}
#endif // ifndef __TCOMSAMPLEADAPTIVEOFFSET__
