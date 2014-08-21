/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <chenm003@163.com>
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_SAO_H
#define X265_SAO_H

#include "common.h"
#include "frame.h"
#include "entropy.h"

namespace x265 {
// private namespace

enum SAOTypeLen
{
    SAO_EO_LEN = 4,
    SAO_BO_LEN = 4,
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

class SAO
{
protected:

    enum { SAO_MAX_DEPTH = 4 };
    enum { SAO_BO_BITS  = 5 };
    enum { LUMA_GROUP_NUM = 1 << SAO_BO_BITS };
    enum { MAX_NUM_SAO_OFFSETS = 4 };
    enum { MAX_NUM_SAO_CLASS = 33 };

    static const uint32_t s_maxDepth;
    static const int      s_numCulPartsLevel[5];
    static const uint32_t s_eoTable[9];
    static const int      s_numClass[MAX_NUM_SAO_TYPE];

    Entropy     m_rdEntropyCoders[5][CI_NUM_SAO];
    Entropy*    m_entropyCoder;

    int64_t  ***m_count;        // [MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]
    int64_t  ***m_offset;       // [MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]
    int64_t  ***m_offsetOrg;    // [MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE]
    int64_t   (*m_countPreDblk)[3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];     // [LCU][plane][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]
    int64_t   (*m_offsetOrgPreDblk)[3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]; // [LCU][plane][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]
    int64_t   **m_rate;         // [MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE]
    int64_t   **m_dist;         // [MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE]
    double    **m_cost;         // [MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE]
    double     *m_costPartBest; // [MAX_NUM_SAO_PART]
    int64_t    *m_distOrg;      // [MAX_NUM_SAO_PART]
    int        *m_typePartBest; // [MAX_NUM_SAO_PART]
    double      m_depthSaoRate[2][4];
    int32_t*    m_offsetBo;
    int32_t*    m_chromaOffsetBo;
    int8_t      m_offsetEo[LUMA_GROUP_NUM];

    /* TODO: these are dups */
    int         m_picWidth;
    int         m_picHeight;
    uint32_t    m_maxSplitLevel;
    uint32_t    m_maxCUWidth;
    uint32_t    m_maxCUHeight;
    int         m_numCuInWidth;
    int         m_numCuInHeight;
    int         m_numTotalParts;
    int         m_hChromaShift;
    int         m_vChromaShift;

    /* TODO: compile-time */
    uint32_t    m_saoBitIncreaseY;
    uint32_t    m_saoBitIncreaseC; // for chroma
    int         m_offsetThY;
    int         m_offsetThC;

    pixel*      m_clipTable;
    pixel*      m_clipTableBase;
    pixel*      m_lumaTableBo;
    pixel*      m_chromaClipTable;
    pixel*      m_chromaClipTableBase;
    pixel*      m_chromaTableBo;

    /* TODO: likely not necessary */
    TComPicYuv* m_tmpYuv;

    pixel*      m_tmpU1[3];
    pixel*      m_tmpU2[3];
    pixel*      m_tmpL1;
    pixel*      m_tmpL2;

public:

    Frame*      m_pic;
    x265_param* m_param;
    int         m_refDepth;
    int         m_numNoSao[2];
    
    uint32_t    m_qp;
    double      m_lumaLambda;
    double      m_chromaLambda;
    /* TODO: No doubles for distortion */

    SAO();

    void create(x265_param *param);
    void destroy();

    void initSAOParam(SAOParam* saoParam, int partLevel, int partRow, int partCol, int parentPartIdx, int startCUX, int endCUX, int startCUY, int endCUY, int plane) const;
    void allocSaoParam(SAOParam* saoParam) const;
    void resetSAOParam(SAOParam* saoParam);

    void SAOProcess(SAOParam* saoParam);
    pixel* getPicYuvAddr(TComPicYuv* picYuv, int plane, int addr = 0); /* TODO: remove default value */

    // LCU-basd SAO process without slice granularity
    void processSaoCu(int addr, int partIdx, int plane);

    void resetLcuPart(SaoLcuParam* saoLcuParam);
    void convertQT2SaoUnit(SAOParam* saoParam, uint32_t partIdx, int plane);
    void convertOnePart2SaoUnit(SAOParam *saoParam, uint32_t partIdx, int plane);
    void processSaoUnitAll(SaoLcuParam* saoLcuParam, bool oneUnitFlag, int plane);
    void processSaoUnitRow(SaoLcuParam* saoLcuParam, int idxY, int plane);

    void resetSaoUnit(SaoLcuParam* saoUnit);
    void copySaoUnit(SaoLcuParam* saoUnitDst, SaoLcuParam* saoUnitSrc);

    void startSaoEnc(Frame* pic, Entropy* entropyCoder);

    void resetStats();

    void runQuadTreeDecision(SAOQTPart *psQTPart, int partIdx, double &costFinal, int maxLevel, double lambda, int plane);
    void rdoSaoOnePart(SAOQTPart *psQTPart, int partIdx, double lambda, int plane);

    void disablePartTree(SAOQTPart *psQTPart, int partIdx);
    void getSaoStats(SAOQTPart *psQTPart, int plane);
    void calcSaoStatsCu(int addr, int partIdx, int plane);
    void calcSaoStatsCu_BeforeDblk(Frame* pic, int idxX, int idxY);
    void destroyEncBuffer();
    void createEncBuffer();
    void assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, bool &oneUnitFlag);
    void checkMerge(SaoLcuParam* lcuParamCurr, SaoLcuParam * lcuParamCheck, int dir);
    void saoComponentParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft, int plane,
                               double lambda, SaoLcuParam *compSaoParam, double *distortion);
    void sao2ChromaParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft, double lambda,
                            SaoLcuParam *crSaoParam, SaoLcuParam *cbSaoParam, double *distortion);

    inline int64_t estSaoDist(int64_t count, int64_t offset, int64_t offsetOrg, int shift);
    inline int64_t estIterOffset(int typeIdx, int classIdx, double lambda, int64_t offsetInput, int64_t count, int64_t offsetOrg, int shift,
                                 int bitIncrease, int32_t *currentDistortionTableBo, double *currentRdCostTableBo, int offsetTh);
    inline int64_t estSaoTypeDist(int compIdx, int typeIdx, int shift, double lambda, int32_t *currentDistortionTableBo, double *currentRdCostTableBo);

    void rdoSaoUnitRowInit(SAOParam *saoParam);
    void rdoSaoUnitRowEnd(SAOParam *saoParam, int numlcus);
    void rdoSaoUnitRow(SAOParam *saoParam, int idxY);

    static int convertLevelRowCol2Idx(int level, int row, int col);
};

void restoreLFDisabledOrigYuv(Frame* pic);
void origCUSampleRestoration(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth);

}

#endif // ifndef X265_SAO_H
