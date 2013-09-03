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

#ifndef __TENCSAMPLEADAPTIVEOFFSET__
#define __TENCSAMPLEADAPTIVEOFFSET__

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
    TEncBinCABACCounter*** m_binCoderCABAC;          ///< temporal CABAC state storage for RD computation

    Int64  ***m_count;    //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    Int64  ***m_offset;   //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    Int64  ***m_offsetOrg; //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
    Int64  (*m_countPreDblk)[3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];    //[LCU][YCbCr][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    Int64  (*m_offsetOrgPreDblk)[3][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS]; //[LCU][YCbCr][MAX_NUM_SAO_TYPE][MAX_NUM_SAO_CLASS];
    Int64  **m_rate;      //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
    Int64  **m_dist;      //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
    Double **m_cost;      //[MAX_NUM_SAO_PART][MAX_NUM_SAO_TYPE];
    Double *m_costPartBest; //[MAX_NUM_SAO_PART];
    Int64  *m_distOrg;    //[MAX_NUM_SAO_PART];
    Int    *m_typePartBest; //[MAX_NUM_SAO_PART];
    Int     m_offsetThY;
    Int     m_offsetThC;
    Double  m_depthSaoRate[2][4];

public:
    double  lumaLambda;
    double  chromaLambd;
    int     depth;
    Int     numNoSao[2];

    TEncSampleAdaptiveOffset();
    virtual ~TEncSampleAdaptiveOffset();

    void startSaoEnc(TComPic* pic, TEncEntropy* entropyCoder, TEncSbac* rdGoOnSbacCoder);
    void endSaoEnc();
    void resetStats();
    void SAOProcess(SAOParam *saoParam);

    void runQuadTreeDecision(SAOQTPart *psQTPart, Int partIdx, Double &costFinal, Int maxLevel, Double lambda, Int yCbCr);
    void rdoSaoOnePart(SAOQTPart *psQTPart, Int partIdx, Double lambda, Int yCbCr);

    void disablePartTree(SAOQTPart *psQTPart, Int partIdx);
    void getSaoStats(SAOQTPart *psQTPart, Int yCbCr);
    void calcSaoStatsCu(Int addr, Int partIdx, Int yCbCr);
    void calcSaoStatsBlock(Pel* recStart, Pel* orgStart, Int stride, Int64** stats, Int64** counts, UInt width, UInt height, Bool* bBorderAvail, Int yCbCr);
    void calcSaoStatsRowCus_BeforeDblk(TComPic* pic, Int idxY);
    void destroyEncBuffer();
    void createEncBuffer();
    void assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, Bool &oneUnitFlag);
    void checkMerge(SaoLcuParam* lcuParamCurr, SaoLcuParam * lcuParamCheck, Int dir);
    void saoComponentParamDist(Int allowMergeLeft, Int allowMergeUp, SAOParam *saoParam, Int addr, Int addrUp, Int addrLeft, Int yCbCr, Double lambda, SaoLcuParam *compSaoParam, Double *distortion);
    void sao2ChromaParamDist(Int allowMergeLeft, Int allowMergeUp, SAOParam *saoParam, Int addr, Int addrUp, Int addrLeft, Double lambda, SaoLcuParam *crSaoParam, SaoLcuParam *cbSaoParam, Double *distortion);
    inline Int64 estSaoDist(Int64 count, Int64 offset, Int64 offsetOrg, Int shift);
    inline Int64 estIterOffset(Int typeIdx, Int classIdx, Double lambda, Int64 offsetInput, Int64 count, Int64 offsetOrg, Int shift, Int bitIncrease, Int *currentDistortionTableBo, Double *currentRdCostTableBo, Int offsetTh);
    inline Int64 estSaoTypeDist(Int compIdx, Int typeIdx, Int shift, Double lambda, Int *currentDistortionTableBo, Double *currentRdCostTableBo);
    void setMaxNumOffsetsPerPic(Int val) { m_maxNumOffsetsPerPic = val; }

    Int  getMaxNumOffsetsPerPic() { return m_maxNumOffsetsPerPic; }

    void rdoSaoUnitRowInit(SAOParam *saoParam);
    void rdoSaoUnitRowEnd(SAOParam *saoParam, int numlcus);
    void rdoSaoUnitRow(SAOParam *saoParam, Int idxY);
};
}
//! \}

#endif // ifndef __TENCSAMPLEADAPTIVEOFFSET__
