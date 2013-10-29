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

#ifndef X265_TCOMSAMPLEADAPTIVEOFFSET_H
#define X265_TCOMSAMPLEADAPTIVEOFFSET_H

#include "CommonDef.h"
#include "TComPic.h"

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

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
    int         bestType;
    int         length;
    int         subTypeIdx;                ///< indicates EO class or BO band position
    int         offset[4];
    int         startCUX;
    int         startCUY;
    int         endCUX;
    int         endCUY;

    int         partIdx;
    int         partLevel;
    int         partCol;
    int         partRow;

    int         downPartsIdx[NUM_DOWN_PART];
    int         upPartIdx;

    bool        bSplit;

    //---- encoder only start -----//
    bool        bProcessed;
    double      minCost;
    Int64       minDist;
    int         minRate;
    //---- encoder only end -----//
} SAOQTPart;

typedef struct _SaoLcuParam
{
    bool       mergeUpFlag;
    bool       mergeLeftFlag;
    int        typeIdx;
    int        subTypeIdx;                ///< indicates EO class or BO band position
    int        offset[4];
    int        partIdx;
    int        partIdxTmp;
    int        length;
} SaoLcuParam;

struct SAOParam
{
    bool       bSaoFlag[2];
    SAOQTPart* saoPart[3];
    int        maxSplitLevel;
    bool         oneUnitFlag[3];
    SaoLcuParam* saoLcuParam[3];
    int          numCuInHeight;
    int          numCuInWidth;
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

    static const uint32_t m_maxDepth;
    static const int m_numCulPartsLevel[5];
    static const uint32_t m_eoTable[9];
    static const int m_numClass[MAX_NUM_SAO_TYPE];

    int32_t *m_offsetBo;
    int32_t *m_chromaOffsetBo;
    int m_offsetEo[LUMA_GROUP_NUM];

    int  m_picWidth;
    int  m_picHeight;
    uint32_t m_maxSplitLevel;
    uint32_t m_maxCUWidth;
    uint32_t m_maxCUHeight;
    int  m_numCuInWidth;
    int  m_numCuInHeight;
    int  m_numTotalParts;

    uint32_t m_saoBitIncreaseY;
    uint32_t m_saoBitIncreaseC; //for chroma
    uint32_t m_qp;

    Pel   *m_clipTable;
    Pel   *m_clipTableBase;
    Pel   *m_lumaTableBo;
    Pel   *m_chromaClipTable;
    Pel   *m_chromaClipTableBase;
    Pel   *m_chromaTableBo;
    int32_t    *m_upBuff1;
    int32_t    *m_upBuff2;
    int32_t    *m_upBufft;
    TComPicYuv* m_tmpYuv;  //!< temporary picture buffer pointer when non-across slice/tile boundary SAO is enabled

    Pel* m_tmpU1[3];
    Pel* m_tmpU2[3];
    Pel* m_tmpL1;
    Pel* m_tmpL2;
    int     m_maxNumOffsetsPerPic;
    bool    m_saoLcuBoundary;
    bool    m_saoLcuBasedOptimization;

public:

    TComSampleAdaptiveOffset();
    virtual ~TComSampleAdaptiveOffset();

    void create(uint32_t sourceWidth, uint32_t sourceHeight, uint32_t maxCUWidth, uint32_t maxCUHeight);
    void destroy();

    int  convertLevelRowCol2Idx(int level, int row, int col) const;

    void initSAOParam(SAOParam* saoParam, int partLevel, int partRow, int partCol, int parentPartIdx, int startCUX, int endCUX, int startCUY, int endCUY, int yCbCr) const;
    void allocSaoParam(SAOParam* saoParam) const;
    void resetSAOParam(SAOParam* saoParam);
    static void freeSaoParam(SAOParam* saoParam);

    void SAOProcess(SAOParam* saoParam);
    Pel* getPicYuvAddr(TComPicYuv* picYuv, int yCbCr, int addr = 0);

    void processSaoCu(int addr, int partIdx, int yCbCr); //!< LCU-basd SAO process without slice granularity
    void createPicSaoInfo(TComPic* pic);
    void destroyPicSaoInfo();

    void resetLcuPart(SaoLcuParam* saoLcuParam);
    void convertQT2SaoUnit(SAOParam* saoParam, uint32_t partIdx, int yCbCr);
    void convertOnePart2SaoUnit(SAOParam *saoParam, uint32_t partIdx, int yCbCr);
    void processSaoUnitAll(SaoLcuParam* saoLcuParam, bool oneUnitFlag, int yCbCr);
    void processSaoUnitRow(SaoLcuParam* saoLcuParam, int idxY, int yCbCr);
    void setSaoLcuBoundary(int bVal)  { m_saoLcuBoundary = bVal != 0; }

    bool getSaoLcuBoundary()           { return m_saoLcuBoundary; }

    void setSaoLcuBasedOptimization(int bVal)  { m_saoLcuBasedOptimization = bVal != 0; }

    bool getSaoLcuBasedOptimization()           { return m_saoLcuBasedOptimization; }

    void resetSaoUnit(SaoLcuParam* saoUnit);
    void copySaoUnit(SaoLcuParam* saoUnitDst, SaoLcuParam* saoUnitSrc);
};

void PCMLFDisableProcess(TComPic* pic);
void xPCMCURestoration(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth);
}

//! \}
#endif // ifndef X265_TCOMSAMPLEADAPTIVEOFFSET_H
