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

/** \file     TEncSbac.h
    \brief    Context-adaptive entropy encoder class (header)
*/

#ifndef X265_TENCSBAC_H
#define X265_TENCSBAC_H

#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/ContextTables.h"
#include "TEncEntropy.h"
#include "TEncBinCoderCABAC.h"
#include "SyntaxElementWriter.h"

namespace x265 {
// private namespace

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// SBAC encoder class
class TEncSbac : public SyntaxElementWriter, public TEncEntropyIf
{
public:

    TEncSbac();
    virtual ~TEncSbac();

    void  init(TEncBinCABAC* p)       { m_binIf = p; }

    void  uninit()                    { m_binIf = 0; }

    //  Virtual list
    void  resetEntropy();
    void  determineCabacInitIdx();
    void  setBitstream(TComBitIf* p)
    {
        m_bitIf = p;
        // NOTE: When write header, it isn't initial
        if (m_binIf)
            m_binIf->init(p);
    }

    void  setSlice(TComSlice* p)      { m_slice = p; }

    // SBAC RD
    void  resetCoeffCost()            { m_coeffCost = 0;  }

    uint32_t  getCoeffCost()              { return m_coeffCost;  }

    void  load(TEncSbac* scr);
    void  loadIntraDirModeLuma(TEncSbac* scr);
    void  store(TEncSbac* dest);
    void  loadContexts(TEncSbac* scr);
    void  resetBits()                { m_binIf->resetBits(); m_bitIf->resetBits(); }

    uint32_t  getNumberOfWrittenBits()   { return m_binIf->getNumWrittenBits(); }

    //--SBAC RD

    void  codeVPS(TComVPS* vps);
    void  codeSPS(TComSPS* sps);
    void  codePPS(TComPPS* pps);
    void  codeVUI(TComVUI* vui, TComSPS* sps);
    void  codeSliceHeader(TComSlice* slice);
    void  codePTL(TComPTL* ptl, bool profilePresentFlag, int maxNumSubLayersMinus1);
    void  codeProfileTier(ProfileTierLevel* ptl);
    void  codeHrdParameters(TComHRD* hrd, bool commonInfPresentFlag, uint32_t maxNumSubLayersMinus1);
    void  codeTilesWPPEntryPoint(TComSlice* slice);
    void  codeTerminatingBit(uint32_t lsLast);
    void  codeSliceFinish();
    void  codeSaoMaxUvlc(uint32_t code, uint32_t maxSymbol);
    void  codeSaoMerge(uint32_t code);
    void  codeSaoTypeIdx(uint32_t code);
    void  codeSaoUflc(uint32_t length, uint32_t code);
    void codeShortTermRefPicSet(TComReferencePictureSet* pcRPS, bool calledFromSliceHeader, int idx);
    bool findMatchingLTRP(TComSlice* slice, uint32_t *ltrpsIndex, int ltrpPOC, bool usedFlag);
    void xCodePredWeightTable(TComSlice* slice);

    /** code SAO offset sign
     * \param code sign value
     */
    void codeSAOSign(uint32_t code)
    {
        m_binIf->encodeBinEP(code);
    }

    void  codeScalingList(TComScalingList*);

private:

    void  xWriteUnarySymbol(uint32_t symbol, ContextModel* scmModel, int offset);
    void  xWriteUnaryMaxSymbol(uint32_t symbol, ContextModel* scmModel, int offset, uint32_t maxSymbol);
    void  xWriteEpExGolomb(uint32_t symbol, uint32_t count);
    void  xWriteCoefRemainExGolomb(uint32_t symbol, uint32_t &param);

    void  xCopyFrom(TEncSbac* src);
    void  xCopyContextsFrom(TEncSbac* src);

    void codeDFFlag(uint32_t /*code*/, const char* /*symbolName*/);
    void codeDFSvlc(int /*code*/, const char* /*symbolName*/);
    void xCodeScalingList(TComScalingList* scalingList, uint32_t sizeId, uint32_t listId);

public:

    TComSlice*    m_slice;
    TEncBinCABAC* m_binIf;
    //SBAC RD
    uint32_t          m_coeffCost;

    //--Adaptive loop filter

public:

    void codeCUTransquantBypassFlag(TComDataCU* cu, uint32_t absPartIdx);
    void codeSkipFlag(TComDataCU* cu, uint32_t absPartIdx);
    void codeMergeFlag(TComDataCU* cu, uint32_t absPartIdx);
    void codeMergeIndex(TComDataCU* cu, uint32_t absPartIdx);
    void codeSplitFlag(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth);
    void codeMVPIdx(TComDataCU* cu, uint32_t absPartIdx, int eRefList);

    void codePartSize(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth);
    void codePredMode(TComDataCU* cu, uint32_t absPartIdx);
    void codeIPCMInfo(TComDataCU* cu, uint32_t absPartIdx);
    void codeTransformSubdivFlag(uint32_t symbol, uint32_t ctx);
    void codeQtCbf(TComDataCU* cu, uint32_t absPartIdx, TextType ttype, uint32_t trDepth);
    void codeQtRootCbf(TComDataCU* cu, uint32_t absPartIdx);
    void codeQtCbfZero(TComDataCU* cu, TextType ttype, uint32_t trDepth);
    void codeQtRootCbfZero(TComDataCU* cu);
    void codeIntraDirLumaAng(TComDataCU* cu, uint32_t absPartIdx, bool isMultiple);

    void codeIntraDirChroma(TComDataCU* cu, uint32_t absPartIdx);
    void codeInterDir(TComDataCU* cu, uint32_t absPartIdx);
    void codeRefFrmIdx(TComDataCU* cu, uint32_t absPartIdx, int eRefList);
    void codeMvd(TComDataCU* cu, uint32_t absPartIdx, int eRefList);

    void codeDeltaQP(TComDataCU* cu, uint32_t absPartIdx);

    void codeLastSignificantXY(uint32_t posx, uint32_t posy, int width, int height, TextType ttype, uint32_t scanIdx);
    void codeCoeffNxN(TComDataCU* cu, TCoeff* coef, uint32_t absPartIdx, uint32_t width, uint32_t height, uint32_t depth, TextType ttype);
    void codeTransformSkipFlags(TComDataCU* cu, uint32_t absPartIdx, uint32_t width, uint32_t height, TextType ttype);

    // -------------------------------------------------------------------------------------------------------------------
    // for RD-optimizatioon
    // -------------------------------------------------------------------------------------------------------------------

    void estBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype);
    void estCBFBit(estBitsSbacStruct* estBitsSbac);
    void estSignificantCoeffGroupMapBit(estBitsSbacStruct* estBitsSbac, TextType ttype);
    void estSignificantMapBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype);
    void estSignificantCoefficientsBit(estBitsSbacStruct* estBitsSbac, TextType ttype);

    TEncBinCABAC* getEncBinIf()  { return m_binIf; }

private:

    uint32_t                 m_lastQp;
    ContextModel         m_contextModels[MAX_NUM_CTX_MOD];
};
}
//! \}

#endif // ifndef X265_TENCSBAC_H
