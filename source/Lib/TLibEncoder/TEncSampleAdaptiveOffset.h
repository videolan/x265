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

/**
 \file     TEncSampleAdaptiveOffset.h
 \brief    estimation part of sample adaptive offset class (header)
 */

#ifndef X265_TENCSAMPLEADAPTIVEOFFSET_H
#define X265_TENCSAMPLEADAPTIVEOFFSET_H

#include "TLibCommon/TComSampleAdaptiveOffset.h"
#include "TLibCommon/TComPic.h"

#include "TEncEntropy.h"
#include "TEncSbac.h"
#include "TLibCommon/TComBitCounter.h"

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

namespace x265 {
// private namespace

class TEncSampleAdaptiveOffset : public TComSampleAdaptiveOffset
{
private:

    TEncEntropy*      m_entropyCoder;
    TEncSbac***       m_rdSbacCoders;            ///< for CABAC
    TEncSbac*         m_rdGoOnSbacCoder;
    TEncBinCABAC***   m_binCoderCABAC;          ///< temporal CABAC state storage for RD computation

    Int64  ***m_count;    //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    Int64  ***m_offset;   //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    Int64  ***m_offsetOrg; //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
    Int64(*m_countPreDblk)[3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];    //[LCU][YCbCr][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    Int64(*m_offsetOrgPreDblk)[3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]; //[LCU][YCbCr][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    Int64  **m_rate;      //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
    Int64  **m_dist;      //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
    double **m_cost;      //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
    double *m_costPartBest; //[MAX_NUM_SAO_PART];
    Int64  *m_distOrg;    //[MAX_NUM_SAO_PART];
    int    *m_typePartBest; //[MAX_NUM_SAO_PART];
    int     m_offsetThY;
    int     m_offsetThC;
    double  m_depthSaoRate[2][4];

public:

    double  lumaLambda;
    double  chromaLambda;
    int     depth;
    int     numNoSao[2];

    TEncSampleAdaptiveOffset();
    virtual ~TEncSampleAdaptiveOffset();

    void startSaoEnc(TComPic* pic, TEncEntropy* entropyCoder, TEncSbac* rdGoOnSbacCoder);
    void endSaoEnc();
    void resetStats();
    void SAOProcess(SAOParam *saoParam);

    void runQuadTreeDecision(SAOQTPart *psQTPart, int partIdx, double &costFinal, int maxLevel, double lambda, int yCbCr);
    void rdoSaoOnePart(SAOQTPart *psQTPart, int partIdx, double lambda, int yCbCr);

    void disablePartTree(SAOQTPart *psQTPart, int partIdx);
    void getSaoStats(SAOQTPart *psQTPart, int yCbCr);
    void calcSaoStatsCu(int addr, int partIdx, int yCbCr);
    void calcSaoStatsBlock(Pel* recStart, Pel* orgStart, int stride, Int64** stats, Int64** counts, UInt width, UInt height, bool* bBorderAvail, int yCbCr);
    void calcSaoStatsRowCus_BeforeDblk(TComPic* pic, int idxY);
    void destroyEncBuffer();
    void createEncBuffer();
    void assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, bool &oneUnitFlag);
    void checkMerge(SaoLcuParam* lcuParamCurr, SaoLcuParam * lcuParamCheck, int dir);
    void saoComponentParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft, int yCbCr, double lambda, SaoLcuParam *compSaoParam, double *distortion);
    void sao2ChromaParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft, double lambda, SaoLcuParam *crSaoParam, SaoLcuParam *cbSaoParam, double *distortion);
    inline Int64 estSaoDist(Int64 count, Int64 offset, Int64 offsetOrg, int shift);
    inline Int64 estIterOffset(int typeIdx, int classIdx, double lambda, Int64 offsetInput, Int64 count, Int64 offsetOrg, int shift, int bitIncrease, int *currentDistortionTableBo, double *currentRdCostTableBo, int offsetTh);
    inline Int64 estSaoTypeDist(int compIdx, int typeIdx, int shift, double lambda, int *currentDistortionTableBo, double *currentRdCostTableBo);
    void setMaxNumOffsetsPerPic(int val) { m_maxNumOffsetsPerPic = val; }

    int  getMaxNumOffsetsPerPic() { return m_maxNumOffsetsPerPic; }

    void rdoSaoUnitRowInit(SAOParam *saoParam);
    void rdoSaoUnitRowEnd(SAOParam *saoParam, int numlcus);
    void rdoSaoUnitRow(SAOParam *saoParam, int idxY);
};
}
//! \}

#endif // ifndef X265_TENCSAMPLEADAPTIVEOFFSET_H
