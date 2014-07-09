/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Steve Borho <steve@borho.org>
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

#ifndef X265_ENTROPY_H
#define X265_ENTROPY_H

#include "common.h"
#include "bitstream.h"
#include "frame.h"

#include "TLibCommon/TComSlice.h"
#include "TLibCommon/TComDataCU.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"

#include "TLibEncoder/TEncBinCoderCabac.h"

namespace x265 {
// private namespace

#define DONT_SPLIT            0
#define VERTICAL_SPLIT        1
#define QUAD_SPLIT            2
#define NUMBER_OF_SPLIT_MODES 3

static const uint32_t partIdxStepShift[NUMBER_OF_SPLIT_MODES] = { 0, 1, 2 };
static const uint32_t NUMBER_OF_SECTIONS[NUMBER_OF_SPLIT_MODES] = { 1, 2, 4 };

struct TURecurse
{
    uint32_t m_section;
    uint32_t m_splitMode;
    uint32_t m_absPartIdxTURelCU;
    uint32_t m_absPartIdxStep;
};

class SBac : public SyntaxElementWriter
{
public:

    uint64_t      m_pad;
    ContextModel  m_contextModels[MAX_OFF_CTX_MOD];

    TComSlice*    m_slice;
    TEncBinCABAC* m_cabac;

    SBac();
    virtual ~SBac() {}

    void  init(TEncBinCABAC* p)       { m_cabac = p; }
    TEncBinCABAC* getEncBinIf()       { return m_cabac; }

    void  setSlice(TComSlice* p)      { m_slice = p; }

    void  resetBits()                 { m_cabac->resetBits(); m_bitIf->resetBits(); }

    uint32_t getNumberOfWrittenBits() { return m_cabac->getNumWrittenBits(); }

    void resetEntropy();
    void determineCabacInitIdx();
    void setBitstream(BitInterface* p);

    // SBAC RD
    void load(SBac* src);
    void loadIntraDirModeLuma(SBac* src);
    void store(SBac* dest);
    void loadContexts(SBac* src);

    void codeVPS(TComVPS* vps);
    void codeSPS(TComSPS* sps);
    void codePPS(TComPPS* pps);
    void codeVUI(TComVUI* vui, TComSPS* sps);
    void codeAUD(TComSlice *slice);
    void codeSliceHeader(TComSlice* slice);
    void codePTL(TComPTL* ptl, bool profilePresentFlag, int maxNumSubLayersMinus1);
    void codeProfileTier(ProfileTierLevel* ptl);
    void codeHrdParameters(TComHRD* hrd, bool commonInfPresentFlag, uint32_t maxNumSubLayersMinus1);
    void codeTilesWPPEntryPoint(TComSlice* slice);
    void codeTerminatingBit(uint32_t lsLast);
    void codeSliceFinish();
    void codeSaoMaxUvlc(uint32_t code, uint32_t maxSymbol);
    void codeSaoMerge(uint32_t code);
    void codeSaoTypeIdx(uint32_t code);
    void codeSaoUflc(uint32_t length, uint32_t code);
    void codeShortTermRefPicSet(TComReferencePictureSet* pcRPS, bool calledFromSliceHeader, int idx);

    void codeSAOSign(uint32_t code) { m_cabac->encodeBinEP(code); }
    void codeScalingList(TComScalingList*);

    void codeCUTransquantBypassFlag(TComDataCU* cu, uint32_t absPartIdx);
    void codeSkipFlag(TComDataCU* cu, uint32_t absPartIdx);
    void codeMergeFlag(TComDataCU* cu, uint32_t absPartIdx);
    void codeMergeIndex(TComDataCU* cu, uint32_t absPartIdx);
    void codeSplitFlag(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth);
    void codeMVPIdx(uint32_t symbol);

    void codePartSize(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth);
    void codePredMode(TComDataCU* cu, uint32_t absPartIdx);
    void codeTransformSubdivFlag(uint32_t symbol, uint32_t ctx);
    void codeQtCbf(TComDataCU* cu, uint32_t absPartIdx, TextType ttype, uint32_t trDepth, uint32_t absPartIdxStep, uint32_t width, uint32_t height, bool lowestLevel);
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

    void codeLastSignificantXY(uint32_t posx, uint32_t posy, uint32_t log2TrSize, TextType ttype, uint32_t scanIdx);
    void codeCoeffNxN(TComDataCU* cu, coeff_t* coef, uint32_t absPartIdx, uint32_t log2TrSize, TextType ttype);
    void codeTransformSkipFlags(TComDataCU* cu, uint32_t absPartIdx, uint32_t trSize, TextType ttype);

    // -------------------------------------------------------------------------------------------------------------------
    // for RD-optimizatioon
    // -------------------------------------------------------------------------------------------------------------------

    void estBit(estBitsSbacStruct* estBitsSbac, int trSize, TextType ttype);
    void estCBFBit(estBitsSbacStruct* estBitsSbac);
    void estSignificantCoeffGroupMapBit(estBitsSbacStruct* estBitsSbac, TextType ttype);
    void estSignificantMapBit(estBitsSbacStruct* estBitsSbac, int trSize, TextType ttype);
    void estSignificantCoefficientsBit(estBitsSbacStruct* estBitsSbac, TextType ttype);

    bool findMatchingLTRP(TComSlice* slice, uint32_t *ltrpsIndex, int ltrpPOC, bool usedFlag);

private:
    void codePredWeightTable(TComSlice* slice);
    void writeUnaryMaxSymbol(uint32_t symbol, ContextModel* scmModel, int offset, uint32_t maxSymbol);
    void writeEpExGolomb(uint32_t symbol, uint32_t count);
    void writeCoefRemainExGolomb(uint32_t symbol, const uint32_t absGoRice);

    void copyFrom(SBac* src);
    void copyContextsFrom(SBac* src);
    void codeScalingList(TComScalingList* scalingList, uint32_t sizeId, uint32_t listId);
};

/// entropy encoder class
class Entropy
{
public:

    SBac*     m_entropyCoder;
    uint32_t  m_bakAbsPartIdx;
    uint32_t  m_bakChromaOffset;
    uint32_t  m_bakAbsPartIdxCU;

    void setEntropyCoder(SBac* e, TComSlice* slice) { m_entropyCoder = e; m_entropyCoder->setSlice(slice); }
    void setBitstream(BitInterface* p)  { m_entropyCoder->setBitstream(p); }
    void resetBits()                    { m_entropyCoder->resetBits();     }
    void resetEntropy()                 { m_entropyCoder->resetEntropy();  }
    void determineCabacInitIdx()        { m_entropyCoder->determineCabacInitIdx(); }
    uint32_t getNumberOfWrittenBits()   { return m_entropyCoder->getNumberOfWrittenBits(); }

    void encodeSliceHeader(TComSlice* slice)         { m_entropyCoder->codeSliceHeader(slice); }
    void encodeTilesWPPEntryPoint(TComSlice* slice)  { m_entropyCoder->codeTilesWPPEntryPoint(slice); }
    void encodeTerminatingBit(uint32_t isLast)       { m_entropyCoder->codeTerminatingBit(isLast); }
    void encodeSliceFinish()                         { m_entropyCoder->codeSliceFinish(); }

    void encodeVPS(TComVPS* vps)                     { m_entropyCoder->codeVPS(vps); }
    void encodeSPS(TComSPS* sps)                     { m_entropyCoder->codeSPS(sps); }
    void encodePPS(TComPPS* pps)                     { m_entropyCoder->codePPS(pps); }
    void encodeAUD(TComSlice* slice)                 { m_entropyCoder->codeAUD(slice); }

    void encodeCUTransquantBypassFlag(TComDataCU* cu, uint32_t absPartIdx) { m_entropyCoder->codeCUTransquantBypassFlag(cu, absPartIdx); }
    void encodeMergeFlag(TComDataCU* cu, uint32_t absPartIdx)              { m_entropyCoder->codeMergeFlag(cu, absPartIdx); }
    void encodeMergeIndex(TComDataCU* cu, uint32_t absPartIdx)             { m_entropyCoder->codeMergeIndex(cu, absPartIdx); }
    void encodeMvdPU(TComDataCU* cu, uint32_t absPartIdx, int list)        { m_entropyCoder->codeMvd(cu, absPartIdx, list); }
    void encodeMVPIdxPU(TComDataCU* cu, uint32_t absPartIdx, int list)     { m_entropyCoder->codeMVPIdx(cu->getMVPIdx(list, absPartIdx)); }
    void encodeIntraDirModeChroma(TComDataCU* cu, uint32_t absPartIdx)     { m_entropyCoder->codeIntraDirChroma(cu, absPartIdx); }
    void encodeTransformSubdivFlag(uint32_t symbol, uint32_t ctx)          { m_entropyCoder->codeTransformSubdivFlag(symbol, ctx); }
    void encodeQtRootCbf(TComDataCU* cu, uint32_t absPartIdx)              { m_entropyCoder->codeQtRootCbf(cu, absPartIdx); }
    void encodeQtRootCbfZero(TComDataCU* cu)                               { m_entropyCoder->codeQtRootCbfZero(cu); }
    void encodeQP(TComDataCU* cu, uint32_t absPartIdx)                     { m_entropyCoder->codeDeltaQP(cu, absPartIdx); }
    void encodeScalingList(TComScalingList* scalingList)                   { m_entropyCoder->codeScalingList(scalingList); }
    void encodeSkipFlag(TComDataCU* cu, uint32_t absPartIdx)               { m_entropyCoder->codeSkipFlag(cu, absPartIdx); }
    void encodePredMode(TComDataCU* cu, uint32_t absPartIdx)               { m_entropyCoder->codePredMode(cu, absPartIdx); }

    void encodeSplitFlag(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth) { m_entropyCoder->codeSplitFlag(cu, absPartIdx, depth); }
    void encodePartSize(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)  { m_entropyCoder->codePartSize(cu, absPartIdx, depth); }

    void encodeIntraDirModeLuma(TComDataCU* cu, uint32_t absPartIdx, bool isMultiplePU = false)
    {
        m_entropyCoder->codeIntraDirLumaAng(cu, absPartIdx, isMultiplePU);
    }
    void encodeQtCbf(TComDataCU* cu, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t width, uint32_t height, TextType ttype, uint32_t trDepth, bool lowestLevel)
    {
        m_entropyCoder->codeQtCbf(cu, absPartIdx, ttype, trDepth, absPartIdxStep, width, height, lowestLevel);
    }
    void encodeQtCbf(TComDataCU* cu, uint32_t absPartIdx, TextType ttype, uint32_t trDepth)
    {
        m_entropyCoder->codeQtCbf(cu, absPartIdx, ttype, trDepth);
    }
    void encodeQtCbfZero(TComDataCU* cu, TextType ttype, uint32_t trDepth)
    {
        m_entropyCoder->codeQtCbfZero(cu, ttype, trDepth);
    }

    void encodePUWise(TComDataCU* cu, uint32_t absPartIdx);
    void encodeRefFrmIdxPU(TComDataCU* subCU, uint32_t absPartIdx, int eRefList);
    void encodePredInfo(TComDataCU* cu, uint32_t absPartIdx);

    void encodeCoeff(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, uint32_t cuSize, bool& bCodeDQP);
    void encodeCoeffNxN(TComDataCU* cu, coeff_t* coeff, uint32_t absPartIdx, uint32_t log2TrSize, TextType ttype)
    {
        m_entropyCoder->codeCoeffNxN(cu, coeff, absPartIdx, log2TrSize, ttype);
    }

    void estimateBit(estBitsSbacStruct* estBitsSbac, int trSize, TextType ttype)
    {
        m_entropyCoder->estBit(estBitsSbac, trSize, ttype == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA);
    }

    void encodeSaoOffset(SaoLcuParam* saoLcuParam, uint32_t compIdx);
    void encodeSaoUnitInterleaving(int compIdx, bool saoFlag, int rx, int ry, SaoLcuParam* saoLcuParam, int cuAddrInSlice, int cuAddrUpInSlice, int allowMergeLeft, int allowMergeUp);

private:

    void initTUEntropySection(TURecurse *TUIterator, uint32_t splitMode, uint32_t absPartIdxStep, uint32_t absPartIdxTU);
    bool isNextTUSection(TURecurse *TUIterator);
    void encodeTransform(TComDataCU* cu, uint32_t offsetLumaOffset, uint32_t offsetChroma, uint32_t absPartIdx, uint32_t absPartIdxStep, uint32_t depth, uint32_t tuSize, uint32_t uiTrIdx, bool& bCodeDQP);
};
}

#endif // ifndef X265_ENTROPY_H
