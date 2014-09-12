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
    enum { SAO_BIT_INC = X265_MAX(X265_DEPTH - 10, 0) };
    enum { OFFSET_THRESH = 1 << X265_MIN(X265_DEPTH - 5, 5) };

    static const int      s_numCulPartsLevel[5];
    static const int      s_numClass[MAX_NUM_SAO_TYPE];
    static const uint32_t s_eoTable[9];

    typedef int64_t (PerClass[MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]);
    typedef int64_t (PerType[MAX_NUM_SAO_TYPE]);
    typedef double  (PerTypeD[MAX_NUM_SAO_TYPE]);
    typedef int64_t (PerPlane[3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]);

    /* allocated per part */
    PerClass*   m_count;
    PerClass*   m_offset;
    PerClass*   m_offsetOrg;
    PerType*    m_rate;
    PerType*    m_dist;
    PerTypeD*   m_cost;
    double*     m_costPartBest;
    int64_t*    m_distOrg;
    int*        m_typePartBest;

    /* allocated per LCU */
    PerPlane*   m_countPreDblk;
    PerPlane*   m_offsetOrgPreDblk;

    double      m_depthSaoRate[2][4];
    int32_t*    m_offsetBo;
    int32_t*    m_chromaOffsetBo;
    int8_t      m_offsetEo[LUMA_GROUP_NUM];

    int         m_maxSplitLevel;

    int         m_numCuInWidth;
    int         m_numCuInHeight;
    int         m_numTotalParts;
    int         m_hChromaShift;
    int         m_vChromaShift;

    pixel*      m_clipTable;
    pixel*      m_clipTableBase;
    pixel*      m_tableBo;

    pixel*      m_tmpU1[3];
    pixel*      m_tmpU2[3];
    pixel*      m_tmpL1;
    pixel*      m_tmpL2;

public:

    Frame*      m_pic;
    Entropy     m_rdEntropyCoders[5][CI_NUM_SAO];
    Entropy     m_entropyCoder;

    x265_param* m_param;
    int         m_refDepth;
    int         m_numNoSao[2];
    
    double      m_lumaLambda;
    double      m_chromaLambda;
    /* TODO: No doubles for distortion */

    SAO();

    bool create(x265_param *param);
    void destroy();

    void initSAOParam(SAOParam* saoParam, int partLevel, int partRow, int partCol, int parentPartIdx, int startCUX, int endCUX, int startCUY, int endCUY, int plane) const;
    void allocSaoParam(SAOParam* saoParam) const;

    void startSlice(Frame *pic, Entropy& initState, int qp);
    void resetSAOParam(SAOParam* saoParam);
    void resetStats();
    void resetSaoUnit(SaoLcuParam* saoUnit);

    void SAOProcess(SAOParam* saoParam);

    // LCU-basd SAO process without slice granularity
    void processSaoCu(int addr, int partIdx, int plane);

    void resetLcuPart(SaoLcuParam* saoLcuParam);
    void convertQT2SaoUnit(SAOParam* saoParam, uint32_t partIdx, int plane);
    void convertOnePart2SaoUnit(SAOParam *saoParam, uint32_t partIdx, int plane);
    void processSaoUnitAll(SaoLcuParam* saoLcuParam, bool oneUnitFlag, int plane);
    void processSaoUnitRow(SaoLcuParam* saoLcuParam, int idxY, int plane);

    void copySaoUnit(SaoLcuParam* saoUnitDst, SaoLcuParam* saoUnitSrc);

    void runQuadTreeDecision(SAOQTPart *psQTPart, int partIdx, double &costFinal, int maxLevel, int plane);
    void rdoSaoOnePart(SAOQTPart *psQTPart, int partIdx, int plane);

    void disablePartTree(SAOQTPart *psQTPart, int partIdx);
    void getSaoStats(SAOQTPart *psQTPart, int plane);
    void calcSaoStatsCu(int addr, int partIdx, int plane);
    void calcSaoStatsCu_BeforeDblk(Frame* pic, int idxX, int idxY);
    void assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, bool &oneUnitFlag);
    void checkMerge(SaoLcuParam* lcuParamCurr, SaoLcuParam * lcuParamCheck, int dir);

    void saoComponentParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft, int plane,
                               SaoLcuParam *compSaoParam, double *distortion);
    void sao2ChromaParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft,
                            SaoLcuParam *crSaoParam, SaoLcuParam *cbSaoParam, double *distortion);

    inline int64_t estSaoDist(int64_t count, int64_t offset, int64_t offsetOrg, int shift);
    inline int64_t estIterOffset(int typeIdx, int classIdx, double lambda, int64_t offsetInput, int64_t count, int64_t offsetOrg, int shift,
                                 int bitIncrease, int32_t *currentDistortionTableBo, double *currentRdCostTableBo, int offsetTh);
    inline int64_t estSaoTypeDist(int compIdx, int typeIdx, int shift, double lambda, int32_t *currentDistortionTableBo, double *currentRdCostTableBo);

    void rdoSaoUnitRowInit(SAOParam *saoParam);
    void rdoSaoUnitRowEnd(SAOParam *saoParam, int numlcus);
    void rdoSaoUnitRow(SAOParam *saoParam, int idxY);
};

void restoreLFDisabledOrigYuv(Frame* pic);
void origCUSampleRestoration(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth);

}

#endif // ifndef X265_SAO_H
