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
#include "TLibCommon/ContextModel.h"
#include "TEncEntropy.h"
#include "TEncBinCoder.h"
#include "TEncBinCoderCABAC.h"

namespace x265 {
// private namespace

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// SBAC encoder class
class TEncSbac : public TEncEntropyIf
{
public:

    TEncSbac();
    virtual ~TEncSbac();

    void  init(TEncBinIf* p)          { m_binIf = p; }

    void  uninit()                    { m_binIf = 0; }

    //  Virtual list
    void  resetEntropy();
    void  determineCabacInitIdx();
    void  setBitstream(TComBitIf* p)  { m_bitIf = p; m_binIf->init(p); }

    void  setSlice(TComSlice* p)      { m_slice = p; }

    // SBAC RD
    void  resetCoeffCost()            { m_coeffCost = 0;  }

    UInt  getCoeffCost()              { return m_coeffCost;  }

    void  load(TEncSbac* scr);
    void  loadIntraDirModeLuma(TEncSbac* scr);
    void  store(TEncSbac* dest);
    void  loadContexts(TEncSbac* scr);
    void  resetBits()                { m_binIf->resetBits(); m_bitIf->resetBits(); }

    UInt  getNumberOfWrittenBits()   { return m_binIf->getNumWrittenBits(); }

    //--SBAC RD

    void  codeVPS(TComVPS* vps);
    void  codeSPS(TComSPS* sps);
    void  codePPS(TComPPS* pps);
    void  codeSliceHeader(TComSlice* slice);
    void  codeTilesWPPEntryPoint(TComSlice* pSlice);
    void  codeTerminatingBit(UInt lsLast);
    void  codeSliceFinish();
    void  codeSaoMaxUvlc(UInt code, UInt maxSymbol);
    void  codeSaoMerge(UInt code);
    void  codeSaoTypeIdx(UInt code);
    void  codeSaoUflc(UInt length, UInt code);

    /** code SAO offset sign
     * \param code sign value
     */
    void codeSAOSign(UInt code)
    {
        m_binIf->encodeBinEP(code);
    }

    void  codeScalingList(TComScalingList*) { assert(0); }

private:

    void  xWriteUnarySymbol(UInt symbol, ContextModel* scmModel, int offset);
    void  xWriteUnaryMaxSymbol(UInt symbol, ContextModel* scmModel, int offset, UInt maxSymbol);
    void  xWriteEpExGolomb(UInt symbol, UInt count);
    void  xWriteCoefRemainExGolomb(UInt symbol, UInt &param);

    void  xCopyFrom(TEncSbac* src);
    void  xCopyContextsFrom(TEncSbac* src);

    void codeDFFlag(UInt /*code*/, const char* /*symbolName*/) { printf("Not supported in codeDFFlag()\n"); assert(0); }

    void codeDFSvlc(int /*code*/, const char* /*symbolName*/)  { printf("Not supported in codeDFSvlc()\n"); assert(0); }

public:

    TComBitIf*    m_bitIf;
    TComSlice*    m_slice;
    TEncBinIf*    m_binIf;
    //SBAC RD
    UInt          m_coeffCost;

    //--Adaptive loop filter

public:

    void codeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx);
    void codeSkipFlag(TComDataCU* cu, UInt absPartIdx);
    void codeMergeFlag(TComDataCU* cu, UInt absPartIdx);
    void codeMergeIndex(TComDataCU* cu, UInt absPartIdx);
    void codeSplitFlag(TComDataCU* cu, UInt absPartIdx, UInt depth);
    void codeMVPIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);

    void codePartSize(TComDataCU* cu, UInt absPartIdx, UInt depth);
    void codePredMode(TComDataCU* cu, UInt absPartIdx);
    void codeIPCMInfo(TComDataCU* cu, UInt absPartIdx);
    void codeTransformSubdivFlag(UInt symbol, UInt ctx);
    void codeQtCbf(TComDataCU* cu, UInt absPartIdx, TextType ttype, UInt trDepth);
    void codeQtRootCbf(TComDataCU* cu, UInt absPartIdx);
    void codeQtCbfZero(TComDataCU* cu, TextType ttype, UInt trDepth);
    void codeQtRootCbfZero(TComDataCU* cu);
    void codeIntraDirLumaAng(TComDataCU* cu, UInt absPartIdx, bool isMultiple);

    void codeIntraDirChroma(TComDataCU* cu, UInt absPartIdx);
    void codeInterDir(TComDataCU* cu, UInt absPartIdx);
    void codeRefFrmIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);
    void codeMvd(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList);

    void codeDeltaQP(TComDataCU* cu, UInt absPartIdx);

    void codeLastSignificantXY(UInt posx, UInt posy, int width, int height, TextType ttype, UInt scanIdx);
    void codeCoeffNxN(TComDataCU* cu, TCoeff* coef, UInt absPartIdx, UInt width, UInt height, UInt depth, TextType ttype);
    void codeTransformSkipFlags(TComDataCU* cu, UInt absPartIdx, UInt width, UInt height, TextType ttype);

    // -------------------------------------------------------------------------------------------------------------------
    // for RD-optimizatioon
    // -------------------------------------------------------------------------------------------------------------------

    void estBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype);
    void estCBFBit(estBitsSbacStruct* estBitsSbac);
    void estSignificantCoeffGroupMapBit(estBitsSbacStruct* estBitsSbac, TextType ttype);
    void estSignificantMapBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype);
    void estSignificantCoefficientsBit(estBitsSbacStruct* estBitsSbac, TextType ttype);

    TEncBinIf* getEncBinIf()  { return m_binIf; }

private:

    UInt                 m_lastQp;
    ContextModel         m_contextModels[MAX_NUM_CTX_MOD];
};
}
//! \}

#endif // ifndef X265_TENCSBAC_H
