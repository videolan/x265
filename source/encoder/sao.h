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
    SAO_NUM_BO_CLASSES = 32
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
    enum { MAX_NUM_SAO_CLASS = 33 };
    enum { SAO_BIT_INC = X265_MAX(X265_DEPTH - 10, 0) };
    enum { OFFSET_THRESH = 1 << X265_MIN(X265_DEPTH - 5, 5) };
    enum { NUM_EDGETYPE = 5 };
    enum { NUM_PLANE = 3 };

    static const uint32_t s_eoTable[NUM_EDGETYPE];

    typedef int32_t (PerClass[MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]);
    typedef int32_t (PerPlane[NUM_PLANE][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]);

    /* allocated per part */
    PerClass*   m_count;
    PerClass*   m_offset;
    PerClass*   m_offsetOrg;

    /* allocated per CTU */
    PerPlane*   m_countPreDblk;
    PerPlane*   m_offsetOrgPreDblk;

    double      m_depthSaoRate[2][4];
    pixel*      m_offsetBo;
    int8_t      m_offsetEo[NUM_EDGETYPE];

    int         m_numCuInWidth;
    int         m_numCuInHeight;
    int         m_hChromaShift;
    int         m_vChromaShift;

    pixel*      m_clipTable;
    pixel*      m_clipTableBase;

    pixel*      m_tmpU1[3];
    pixel*      m_tmpU2[3];
    pixel*      m_tmpL1;
    pixel*      m_tmpL2;

public:

    struct SAOContexts
    {
        Entropy cur;
        Entropy next;
        Entropy temp;
    };

    Frame*      m_pic;
    Entropy     m_entropyCoder;
    SAOContexts m_rdContexts;

    x265_param* m_param;
    int         m_refDepth;
    int         m_numNoSao[2];

    double      m_lumaLambda;
    double      m_chromaLambda;
    /* TODO: No doubles for distortion */

    SAO();

    bool create(x265_param *param);
    void destroy();

    void allocSaoParam(SAOParam* saoParam) const;

    void startSlice(Frame *pic, Entropy& initState, int qp);
    void resetSAOParam(SAOParam* saoParam);
    void resetStats();
    void resetSaoUnit(SaoCtuParam* saoUnit);

    // CTU-based SAO process without slice granularity
    void processSaoCu(int addr, int typeIdx, int plane);
    void processSaoUnitRow(SaoCtuParam* ctuParam, int idxY, int plane);

    void copySaoUnit(SaoCtuParam* saoUnitDst, SaoCtuParam* saoUnitSrc);

    void calcSaoStatsCu(int addr, int plane);
    void calcSaoStatsCu_BeforeDblk(Frame* pic, int idxX, int idxY);

    void saoComponentParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft,
                               SaoCtuParam *compSaoParam, double *distortion);
    void sao2ChromaParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft,
                             SaoCtuParam *crSaoParam, SaoCtuParam *cbSaoParam, double *distortion);

    inline int estIterOffset(int typeIdx, int classIdx, double lambda, int offset, int32_t count, int32_t offsetOrg,
                             int32_t *currentDistortionTableBo, double *currentRdCostTableBo);
    inline int64_t estSaoTypeDist(int plane, int typeIdx, double lambda, int32_t *currentDistortionTableBo, double *currentRdCostTableBo);

    void rdoSaoUnitRowInit(SAOParam *saoParam);
    void rdoSaoUnitRowEnd(SAOParam *saoParam, int numctus);
    void rdoSaoUnitRow(SAOParam *saoParam, int idxY);
};

void restoreLFDisabledOrigYuv(Frame* pic);
void origCUSampleRestoration(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth);

}

#endif // ifndef X265_SAO_H
