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

/** \file     TEncSbac.cpp
    \brief    SBAC encoder class
*/

#include "TEncSbac.h"

using namespace x265;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSbac::TEncSbac()
// new structure here
    : m_pcBitIf(NULL)
    , m_pcSlice(NULL)
    , m_pcBinIf(NULL)
    , m_uiCoeffCost(0)
    , m_numContextModels(0)
    , m_cCUSplitFlagSCModel(1,          NUM_SPLIT_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUSkipFlagSCModel(1,           NUM_SKIP_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUMergeFlagExtSCModel(1,       NUM_MERGE_FLAG_EXT_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUMergeIdxExtSCModel(1,        NUM_MERGE_IDX_EXT_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUPartSizeSCModel(1,           NUM_PART_SIZE_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUPredModeSCModel(1,           NUM_PRED_MODE_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUIntraPredSCModel(1,          NUM_ADI_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUChromaPredSCModel(1,         NUM_CHROMA_PRED_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUDeltaQpSCModel(1,            NUM_DELTA_QP_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUInterDirSCModel(1,           NUM_INTER_DIR_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCURefPicSCModel(1,             NUM_REF_NO_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUMvdSCModel(1,                NUM_MV_RES_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUQtCbfSCModel(2,              NUM_QT_CBF_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUTransSubdivFlagSCModel(1,    NUM_TRANS_SUBDIV_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUQtRootCbfSCModel(1,          NUM_QT_ROOT_CBF_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUSigCoeffGroupSCModel(2,      NUM_SIG_CG_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUSigSCModel(1,                NUM_SIG_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCuCtxLastX(2,                  NUM_CTX_LAST_FLAG_XY, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCuCtxLastY(2,                  NUM_CTX_LAST_FLAG_XY, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUOneSCModel(1,                NUM_ONE_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUAbsSCModel(1,                NUM_ABS_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cMVPIdxSCModel(1,               NUM_MVP_IDX_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cCUAMPSCModel(1,                NUM_CU_AMP_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cSaoMergeSCModel(1,             NUM_SAO_MERGE_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cSaoTypeIdxSCModel(1,           NUM_SAO_TYPE_IDX_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cTransformSkipSCModel(2,        NUM_TRANSFORMSKIP_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_CUTransquantBypassFlagSCModel(1,NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
{
    assert(m_numContextModels <= MAX_NUM_CTX_MOD);
}

TEncSbac::~TEncSbac()
{}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

void TEncSbac::resetEntropy()
{
    int  iQp              = m_pcSlice->getSliceQp();
    SliceType eSliceType  = m_pcSlice->getSliceType();

    int  encCABACTableIdx = m_pcSlice->getPPS()->getEncCABACTableIdx();

    if (!m_pcSlice->isIntra() && (encCABACTableIdx == B_SLICE || encCABACTableIdx == P_SLICE) && m_pcSlice->getPPS()->getCabacInitPresentFlag())
    {
        eSliceType = (SliceType)encCABACTableIdx;
    }

    m_cCUSplitFlagSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SPLIT_FLAG);

    m_cCUSkipFlagSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SKIP_FLAG);
    m_cCUMergeFlagExtSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_MERGE_FLAG_EXT);
    m_cCUMergeIdxExtSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_MERGE_IDX_EXT);
    m_cCUPartSizeSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_PART_SIZE);
    m_cCUAMPSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_CU_AMP_POS);
    m_cCUPredModeSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_PRED_MODE);
    m_cCUIntraPredSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_INTRA_PRED_MODE);
    m_cCUChromaPredSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_CHROMA_PRED_MODE);
    m_cCUInterDirSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_INTER_DIR);
    m_cCUMvdSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_MVD);
    m_cCURefPicSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_REF_PIC);
    m_cCUDeltaQpSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_DQP);
    m_cCUQtCbfSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_QT_CBF);
    m_cCUQtRootCbfSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_QT_ROOT_CBF);
    m_cCUSigCoeffGroupSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SIG_CG_FLAG);
    m_cCUSigSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SIG_FLAG);
    m_cCuCtxLastX.initBuffer(eSliceType, iQp, (UChar*)INIT_LAST);
    m_cCuCtxLastY.initBuffer(eSliceType, iQp, (UChar*)INIT_LAST);
    m_cCUOneSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_ONE_FLAG);
    m_cCUAbsSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_ABS_FLAG);
    m_cMVPIdxSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_MVP_IDX);
    m_cCUTransSubdivFlagSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_TRANS_SUBDIV_FLAG);
    m_cSaoMergeSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SAO_MERGE_FLAG);
    m_cSaoTypeIdxSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SAO_TYPE_IDX);
    m_cTransformSkipSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_TRANSFORMSKIP_FLAG);
    m_CUTransquantBypassFlagSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG);
    // new structure
    m_uiLastQp = iQp;

    m_pcBinIf->start();
}

/** The function does the following:
 * If current slice type is P/B then it determines the distance of initialisation type 1 and 2 from the current CABAC states and
 * stores the index of the closest table.  This index is used for the next P/B slice when cabac_init_present_flag is true.
 */
void TEncSbac::determineCabacInitIdx()
{
    int  qp              = m_pcSlice->getSliceQp();

    if (!m_pcSlice->isIntra())
    {
        SliceType aSliceTypeChoices[] = { B_SLICE, P_SLICE };

        UInt bestCost             = MAX_UINT;
        SliceType bestSliceType   = aSliceTypeChoices[0];
        for (UInt idx = 0; idx < 2; idx++)
        {
            UInt curCost          = 0;
            SliceType curSliceType  = aSliceTypeChoices[idx];

            curCost  = m_cCUSplitFlagSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SPLIT_FLAG);
            curCost += m_cCUSkipFlagSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SKIP_FLAG);
            curCost += m_cCUMergeFlagExtSCModel.calcCost(curSliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT);
            curCost += m_cCUMergeIdxExtSCModel.calcCost(curSliceType, qp, (UChar*)INIT_MERGE_IDX_EXT);
            curCost += m_cCUPartSizeSCModel.calcCost(curSliceType, qp, (UChar*)INIT_PART_SIZE);
            curCost += m_cCUAMPSCModel.calcCost(curSliceType, qp, (UChar*)INIT_CU_AMP_POS);
            curCost += m_cCUPredModeSCModel.calcCost(curSliceType, qp, (UChar*)INIT_PRED_MODE);
            curCost += m_cCUIntraPredSCModel.calcCost(curSliceType, qp, (UChar*)INIT_INTRA_PRED_MODE);
            curCost += m_cCUChromaPredSCModel.calcCost(curSliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE);
            curCost += m_cCUInterDirSCModel.calcCost(curSliceType, qp, (UChar*)INIT_INTER_DIR);
            curCost += m_cCUMvdSCModel.calcCost(curSliceType, qp, (UChar*)INIT_MVD);
            curCost += m_cCURefPicSCModel.calcCost(curSliceType, qp, (UChar*)INIT_REF_PIC);
            curCost += m_cCUDeltaQpSCModel.calcCost(curSliceType, qp, (UChar*)INIT_DQP);
            curCost += m_cCUQtCbfSCModel.calcCost(curSliceType, qp, (UChar*)INIT_QT_CBF);
            curCost += m_cCUQtRootCbfSCModel.calcCost(curSliceType, qp, (UChar*)INIT_QT_ROOT_CBF);
            curCost += m_cCUSigCoeffGroupSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SIG_CG_FLAG);
            curCost += m_cCUSigSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SIG_FLAG);
            curCost += m_cCuCtxLastX.calcCost(curSliceType, qp, (UChar*)INIT_LAST);
            curCost += m_cCuCtxLastY.calcCost(curSliceType, qp, (UChar*)INIT_LAST);
            curCost += m_cCUOneSCModel.calcCost(curSliceType, qp, (UChar*)INIT_ONE_FLAG);
            curCost += m_cCUAbsSCModel.calcCost(curSliceType, qp, (UChar*)INIT_ABS_FLAG);
            curCost += m_cMVPIdxSCModel.calcCost(curSliceType, qp, (UChar*)INIT_MVP_IDX);
            curCost += m_cCUTransSubdivFlagSCModel.calcCost(curSliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG);
            curCost += m_cSaoMergeSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG);
            curCost += m_cSaoTypeIdxSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SAO_TYPE_IDX);
            curCost += m_cTransformSkipSCModel.calcCost(curSliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG);
            curCost += m_CUTransquantBypassFlagSCModel.calcCost(curSliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG);
            if (curCost < bestCost)
            {
                bestSliceType = curSliceType;
                bestCost      = curCost;
            }
        }

        m_pcSlice->getPPS()->setEncCABACTableIdx(bestSliceType);
    }
    else
    {
        m_pcSlice->getPPS()->setEncCABACTableIdx(I_SLICE);
    }
}

/** The function does the followng: Write out terminate bit. Flush CABAC. Intialize CABAC states. Start CABAC.
 */
void TEncSbac::updateContextTables(SliceType eSliceType, int iQp, bool bExecuteFinish)
{
    m_pcBinIf->encodeBinTrm(1);
    if (bExecuteFinish) m_pcBinIf->finish();
    m_cCUSplitFlagSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SPLIT_FLAG);

    m_cCUSkipFlagSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SKIP_FLAG);
    m_cCUMergeFlagExtSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_MERGE_FLAG_EXT);
    m_cCUMergeIdxExtSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_MERGE_IDX_EXT);
    m_cCUPartSizeSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_PART_SIZE);
    m_cCUAMPSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_CU_AMP_POS);
    m_cCUPredModeSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_PRED_MODE);
    m_cCUIntraPredSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_INTRA_PRED_MODE);
    m_cCUChromaPredSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_CHROMA_PRED_MODE);
    m_cCUInterDirSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_INTER_DIR);
    m_cCUMvdSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_MVD);
    m_cCURefPicSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_REF_PIC);
    m_cCUDeltaQpSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_DQP);
    m_cCUQtCbfSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_QT_CBF);
    m_cCUQtRootCbfSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_QT_ROOT_CBF);
    m_cCUSigCoeffGroupSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SIG_CG_FLAG);
    m_cCUSigSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SIG_FLAG);
    m_cCuCtxLastX.initBuffer(eSliceType, iQp, (UChar*)INIT_LAST);
    m_cCuCtxLastY.initBuffer(eSliceType, iQp, (UChar*)INIT_LAST);
    m_cCUOneSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_ONE_FLAG);
    m_cCUAbsSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_ABS_FLAG);
    m_cMVPIdxSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_MVP_IDX);
    m_cCUTransSubdivFlagSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_TRANS_SUBDIV_FLAG);
    m_cSaoMergeSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SAO_MERGE_FLAG);
    m_cSaoTypeIdxSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_SAO_TYPE_IDX);
    m_cTransformSkipSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_TRANSFORMSKIP_FLAG);
    m_CUTransquantBypassFlagSCModel.initBuffer(eSliceType, iQp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG);
    m_pcBinIf->start();
}

void TEncSbac::codeVPS(TComVPS*)
{
    assert(0);
}

void TEncSbac::codeSPS(TComSPS*)
{
    assert(0);
}

void TEncSbac::codePPS(TComPPS*)
{
    assert(0);
}

void TEncSbac::codeSliceHeader(TComSlice*)
{
    assert(0);
}

void TEncSbac::codeTilesWPPEntryPoint(TComSlice*)
{
    assert(0);
}

void TEncSbac::codeTerminatingBit(UInt uilsLast)
{
    m_pcBinIf->encodeBinTrm(uilsLast);
}

void TEncSbac::codeSliceFinish()
{
    m_pcBinIf->finish();
}

void TEncSbac::xWriteUnarySymbol(UInt uiSymbol, ContextModel* pcSCModel, int offset)
{
    m_pcBinIf->encodeBin(uiSymbol ? 1 : 0, pcSCModel[0]);

    if (0 == uiSymbol)
    {
        return;
    }

    while (uiSymbol--)
    {
        m_pcBinIf->encodeBin(uiSymbol ? 1 : 0, pcSCModel[offset]);
    }
}

void TEncSbac::xWriteUnaryMaxSymbol(UInt uiSymbol, ContextModel* pcSCModel, int offset, UInt uiMaxSymbol)
{
    if (uiMaxSymbol == 0)
    {
        return;
    }

    m_pcBinIf->encodeBin(uiSymbol ? 1 : 0, pcSCModel[0]);

    if (uiSymbol == 0)
    {
        return;
    }

    bool bCodeLast = (uiMaxSymbol > uiSymbol);

    while (--uiSymbol)
    {
        m_pcBinIf->encodeBin(1, pcSCModel[offset]);
    }

    if (bCodeLast)
    {
        m_pcBinIf->encodeBin(0, pcSCModel[offset]);
    }
}

void TEncSbac::xWriteEpExGolomb(UInt uiSymbol, UInt uiCount)
{
    UInt bins = 0;
    int numBins = 0;

    while (uiSymbol >= (UInt)(1 << uiCount))
    {
        bins = 2 * bins + 1;
        numBins++;
        uiSymbol -= 1 << uiCount;
        uiCount++;
    }

    bins = 2 * bins + 0;
    numBins++;

    bins = (bins << uiCount) | uiSymbol;
    numBins += uiCount;

    assert(numBins <= 32);
    m_pcBinIf->encodeBinsEP(bins, numBins);
}

/** Coding of coeff_abs_level_minus3
 * \param uiSymbol value of coeff_abs_level_minus3
 * \param ruiGoRiceParam reference to Rice parameter
 * \returns void
 */
void TEncSbac::xWriteCoefRemainExGolomb(UInt symbol, UInt &rParam)
{
    int codeNumber  = (int)symbol;
    UInt length;

    if (codeNumber < (COEF_REMAIN_BIN_REDUCTION << rParam))
    {
        length = codeNumber >> rParam;
        m_pcBinIf->encodeBinsEP((1 << (length + 1)) - 2, length + 1);
        m_pcBinIf->encodeBinsEP((codeNumber % (1 << rParam)), rParam);
    }
    else
    {
        length = rParam;
        codeNumber  = codeNumber - (COEF_REMAIN_BIN_REDUCTION << rParam);
        while (codeNumber >= (1 << length))
        {
            codeNumber -=  (1 << (length++));
        }

        m_pcBinIf->encodeBinsEP((1 << (COEF_REMAIN_BIN_REDUCTION + length + 1 - rParam)) - 2, COEF_REMAIN_BIN_REDUCTION + length + 1 - rParam);
        m_pcBinIf->encodeBinsEP(codeNumber, length);
    }
}

// SBAC RD
void  TEncSbac::load(TEncSbac* src)
{
    this->xCopyFrom(src);
}

void  TEncSbac::loadIntraDirModeLuma(TEncSbac* src)
{
    m_pcBinIf->copyState(src->m_pcBinIf);

    this->m_cCUIntraPredSCModel.copyFrom(&src->m_cCUIntraPredSCModel);
}

void  TEncSbac::store(TEncSbac* pDest)
{
    pDest->xCopyFrom(this);
}

void TEncSbac::xCopyFrom(TEncSbac* src)
{
    m_pcBinIf->copyState(src->m_pcBinIf);

    this->m_uiCoeffCost = src->m_uiCoeffCost;
    this->m_uiLastQp    = src->m_uiLastQp;

    memcpy(m_contextModels, src->m_contextModels, m_numContextModels * sizeof(ContextModel));
}

void TEncSbac::codeMVPIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList)
{
    int iSymbol = cu->getMVPIdx(eRefList, absPartIdx);
    int iNum = AMVP_MAX_NUM_CANDS;

    xWriteUnaryMaxSymbol(iSymbol, &m_cMVPIdxSCModel.get(0), 1, iNum - 1);
}

void TEncSbac::codePartSize(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    PartSize partSize         = cu->getPartitionSize(absPartIdx);

    if (cu->isIntra(absPartIdx))
    {
        if (depth == g_maxCUDepth - g_addCUDepth)
        {
            m_pcBinIf->encodeBin(partSize == SIZE_2Nx2N ? 1 : 0, m_cCUPartSizeSCModel.get(0));
        }
        return;
    }

    switch (partSize)
    {
    case SIZE_2Nx2N:
    {
        m_pcBinIf->encodeBin(1, m_cCUPartSizeSCModel.get(0));
        break;
    }
    case SIZE_2NxN:
    case SIZE_2NxnU:
    case SIZE_2NxnD:
    {
        m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get(0));
        m_pcBinIf->encodeBin(1, m_cCUPartSizeSCModel.get(1));
        if (cu->getSlice()->getSPS()->getAMPAcc(depth))
        {
            if (partSize == SIZE_2NxN)
            {
                m_pcBinIf->encodeBin(1, m_cCUAMPSCModel.get(0));
            }
            else
            {
                m_pcBinIf->encodeBin(0, m_cCUAMPSCModel.get(0));
                m_pcBinIf->encodeBinEP((partSize == SIZE_2NxnU ? 0 : 1));
            }
        }
        break;
    }
    case SIZE_Nx2N:
    case SIZE_nLx2N:
    case SIZE_nRx2N:
    {
        m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get(0));
        m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get(1));
        if (depth == g_maxCUDepth - g_addCUDepth && !(cu->getWidth(absPartIdx) == 8 && cu->getHeight(absPartIdx) == 8))
        {
            m_pcBinIf->encodeBin(1, m_cCUPartSizeSCModel.get(2));
        }
        if (cu->getSlice()->getSPS()->getAMPAcc(depth))
        {
            if (partSize == SIZE_Nx2N)
            {
                m_pcBinIf->encodeBin(1, m_cCUAMPSCModel.get(0));
            }
            else
            {
                m_pcBinIf->encodeBin(0, m_cCUAMPSCModel.get(0));
                m_pcBinIf->encodeBinEP((partSize == SIZE_nLx2N ? 0 : 1));
            }
        }
        break;
    }
    case SIZE_NxN:
    {
        if (depth == g_maxCUDepth - g_addCUDepth && !(cu->getWidth(absPartIdx) == 8 && cu->getHeight(absPartIdx) == 8))
        {
            m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get(0));
            m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get(1));
            m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get(2));
        }
        break;
    }
    default:
    {
        assert(0);
    }
    }
}

/** code prediction mode
 * \param cu
 * \param absPartIdx
 * \returns void
 */
void TEncSbac::codePredMode(TComDataCU* cu, UInt absPartIdx)
{
    // get context function is here
    int iPredMode = cu->getPredictionMode(absPartIdx);

    m_pcBinIf->encodeBin(iPredMode == MODE_INTER ? 0 : 1, m_cCUPredModeSCModel.get(0));
}

void TEncSbac::codeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx)
{
    UInt uiSymbol = cu->getCUTransquantBypass(absPartIdx);

    m_pcBinIf->encodeBin(uiSymbol, m_CUTransquantBypassFlagSCModel.get(0));
}

/** code skip flag
 * \param cu
 * \param absPartIdx
 * \returns void
 */
void TEncSbac::codeSkipFlag(TComDataCU* cu, UInt absPartIdx)
{
    // get context function is here
    UInt uiSymbol = cu->isSkipped(absPartIdx) ? 1 : 0;
    UInt uiCtxSkip = cu->getCtxSkipFlag(absPartIdx);

    m_pcBinIf->encodeBin(uiSymbol, m_cCUSkipFlagSCModel.get(uiCtxSkip));
    DTRACE_CABAC_VL(g_nSymbolCounter++);
    DTRACE_CABAC_T("\tSkipFlag");
    DTRACE_CABAC_T("\tuiCtxSkip: ");
    DTRACE_CABAC_V(uiCtxSkip);
    DTRACE_CABAC_T("\tuiSymbol: ");
    DTRACE_CABAC_V(uiSymbol);
    DTRACE_CABAC_T("\n");
}

/** code merge flag
 * \param cu
 * \param absPartIdx
 * \returns void
 */
void TEncSbac::codeMergeFlag(TComDataCU* cu, UInt absPartIdx)
{
    const UInt uiSymbol = cu->getMergeFlag(absPartIdx) ? 1 : 0;

    m_pcBinIf->encodeBin(uiSymbol, m_cCUMergeFlagExtSCModel.get(0));

    DTRACE_CABAC_VL(g_nSymbolCounter++);
    DTRACE_CABAC_T("\tMergeFlag: ");
    DTRACE_CABAC_V(uiSymbol);
    DTRACE_CABAC_T("\tAddress: ");
    DTRACE_CABAC_V(cu->getAddr());
    DTRACE_CABAC_T("\tuiAbsPartIdx: ");
    DTRACE_CABAC_V(absPartIdx);
    DTRACE_CABAC_T("\n");
}

/** code merge index
 * \param cu
 * \param absPartIdx
 * \returns void
 */
void TEncSbac::codeMergeIndex(TComDataCU* cu, UInt absPartIdx)
{
    UInt uiUnaryIdx = cu->getMergeIndex(absPartIdx);
    UInt uiNumCand = cu->getSlice()->getMaxNumMergeCand();

    if (uiNumCand > 1)
    {
        for (UInt ui = 0; ui < uiNumCand - 1; ++ui)
        {
            const UInt uiSymbol = ui == uiUnaryIdx ? 0 : 1;
            if (ui == 0)
            {
                m_pcBinIf->encodeBin(uiSymbol, m_cCUMergeIdxExtSCModel.get(0));
            }
            else
            {
                m_pcBinIf->encodeBinEP(uiSymbol);
            }
            if (uiSymbol == 0)
            {
                break;
            }
        }
    }
    DTRACE_CABAC_VL(g_nSymbolCounter++);
    DTRACE_CABAC_T("\tparseMergeIndex()");
    DTRACE_CABAC_T("\tuiMRGIdx= ");
    DTRACE_CABAC_V(cu->getMergeIndex(absPartIdx));
    DTRACE_CABAC_T("\n");
}

void TEncSbac::codeSplitFlag(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    if (depth == g_maxCUDepth - g_addCUDepth)
        return;

    UInt uiCtx           = cu->getCtxSplitFlag(absPartIdx, depth);
    UInt uiCurrSplitFlag = (cu->getDepth(absPartIdx) > depth) ? 1 : 0;

    assert(uiCtx < 3);
    m_pcBinIf->encodeBin(uiCurrSplitFlag, m_cCUSplitFlagSCModel.get(uiCtx));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tSplitFlag\n")
}

void TEncSbac::codeTransformSubdivFlag(UInt uiSymbol, UInt uiCtx)
{
    m_pcBinIf->encodeBin(uiSymbol, m_cCUTransSubdivFlagSCModel.get(uiCtx));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseTransformSubdivFlag()")
    DTRACE_CABAC_T("\tsymbol=")
    DTRACE_CABAC_V(uiSymbol)
    DTRACE_CABAC_T("\tctx=")
    DTRACE_CABAC_V(uiCtx)
    DTRACE_CABAC_T("\n")
}

void TEncSbac::codeIntraDirLumaAng(TComDataCU* cu, UInt absPartIdx, bool isMultiple)
{
    UInt dir[4], j;
    int preds[4][3] = { { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } };
    int predNum[4], predIdx[4] = { -1, -1, -1, -1 };
    PartSize mode = cu->getPartitionSize(absPartIdx);
    UInt partNum = isMultiple ? (mode == SIZE_NxN ? 4 : 1) : 1;
    UInt partOffset = (cu->getPic()->getNumPartInCU() >> (cu->getDepth(absPartIdx) << 1)) >> 2;

    for (j = 0; j < partNum; j++)
    {
        dir[j] = cu->getLumaIntraDir(absPartIdx + partOffset * j);
        predNum[j] = cu->getIntraDirLumaPredictor(absPartIdx + partOffset * j, preds[j]);
        for (UInt i = 0; i < predNum[j]; i++)
        {
            if (dir[j] == preds[j][i])
            {
                predIdx[j] = i;
            }
        }

        m_pcBinIf->encodeBin((predIdx[j] != -1) ? 1 : 0, m_cCUIntraPredSCModel.get(0));
    }

    for (j = 0; j < partNum; j++)
    {
        if (predIdx[j] != -1)
        {
            m_pcBinIf->encodeBinEP(predIdx[j] ? 1 : 0);
            if (predIdx[j])
            {
                m_pcBinIf->encodeBinEP(predIdx[j] - 1);
            }
        }
        else
        {
            if (preds[j][0] > preds[j][1])
            {
                std::swap(preds[j][0], preds[j][1]);
            }
            if (preds[j][0] > preds[j][2])
            {
                std::swap(preds[j][0], preds[j][2]);
            }
            if (preds[j][1] > preds[j][2])
            {
                std::swap(preds[j][1], preds[j][2]);
            }
            for (int i = (predNum[j] - 1); i >= 0; i--)
            {
                dir[j] = dir[j] > preds[j][i] ? dir[j] - 1 : dir[j];
            }

            m_pcBinIf->encodeBinsEP(dir[j], 5);
        }
    }
}

void TEncSbac::codeIntraDirChroma(TComDataCU* cu, UInt absPartIdx)
{
    UInt uiIntraDirChroma = cu->getChromaIntraDir(absPartIdx);

    if (uiIntraDirChroma == DM_CHROMA_IDX)
    {
        m_pcBinIf->encodeBin(0, m_cCUChromaPredSCModel.get(0));
    }
    else
    {
        UInt uiAllowedChromaDir[NUM_CHROMA_MODE];
        cu->getAllowedChromaDir(absPartIdx, uiAllowedChromaDir);

        for (int i = 0; i < NUM_CHROMA_MODE - 1; i++)
        {
            if (uiIntraDirChroma == uiAllowedChromaDir[i])
            {
                uiIntraDirChroma = i;
                break;
            }
        }

        m_pcBinIf->encodeBin(1, m_cCUChromaPredSCModel.get(0));

        m_pcBinIf->encodeBinsEP(uiIntraDirChroma, 2);
    }
}

void TEncSbac::codeInterDir(TComDataCU* cu, UInt absPartIdx)
{
    const UInt uiInterDir = cu->getInterDir(absPartIdx) - 1;
    const UInt uiCtx      = cu->getCtxInterDir(absPartIdx);

    if (cu->getPartitionSize(absPartIdx) == SIZE_2Nx2N || cu->getHeight(absPartIdx) != 8)
    {
        m_pcBinIf->encodeBin(uiInterDir == 2 ? 1 : 0, m_cCUInterDirSCModel.get(uiCtx));
    }
    if (uiInterDir < 2)
    {
        m_pcBinIf->encodeBin(uiInterDir, m_cCUInterDirSCModel.get(4));
    }
}

void TEncSbac::codeRefFrmIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList)
{
    {
        int iRefFrame = cu->getCUMvField(eRefList)->getRefIdx(absPartIdx);
        int idx = 0;
        m_pcBinIf->encodeBin((iRefFrame == 0 ? 0 : 1), m_cCURefPicSCModel.get(0));

        if (iRefFrame > 0)
        {
            UInt uiRefNum = cu->getSlice()->getNumRefIdx(eRefList) - 2;
            idx++;
            iRefFrame--;
            for (UInt ui = 0; ui < uiRefNum; ++ui)
            {
                const UInt uiSymbol = ui == iRefFrame ? 0 : 1;
                if (ui == 0)
                {
                    m_pcBinIf->encodeBin(uiSymbol, m_cCURefPicSCModel.get(idx));
                }
                else
                {
                    m_pcBinIf->encodeBinEP(uiSymbol);
                }
                if (uiSymbol == 0)
                {
                    break;
                }
            }
        }
    }
}

void TEncSbac::codeMvd(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList)
{
    if (cu->getSlice()->getMvdL1ZeroFlag() && eRefList == REF_PIC_LIST_1 && cu->getInterDir(absPartIdx) == 3)
    {
        return;
    }

    const TComCUMvField* pcCUMvField = cu->getCUMvField(eRefList);
    const int iHor = pcCUMvField->getMvd(absPartIdx).x;
    const int iVer = pcCUMvField->getMvd(absPartIdx).y;

    m_pcBinIf->encodeBin(iHor != 0 ? 1 : 0, m_cCUMvdSCModel.get(0));
    m_pcBinIf->encodeBin(iVer != 0 ? 1 : 0, m_cCUMvdSCModel.get(0));

    const bool bHorAbsGr0 = iHor != 0;
    const bool bVerAbsGr0 = iVer != 0;
    const UInt uiHorAbs   = 0 > iHor ? -iHor : iHor;
    const UInt uiVerAbs   = 0 > iVer ? -iVer : iVer;

    if (bHorAbsGr0)
    {
        m_pcBinIf->encodeBin(uiHorAbs > 1 ? 1 : 0, m_cCUMvdSCModel.get(1));
    }

    if (bVerAbsGr0)
    {
        m_pcBinIf->encodeBin(uiVerAbs > 1 ? 1 : 0, m_cCUMvdSCModel.get(1));
    }

    if (bHorAbsGr0)
    {
        if (uiHorAbs > 1)
        {
            xWriteEpExGolomb(uiHorAbs - 2, 1);
        }

        m_pcBinIf->encodeBinEP(0 > iHor ? 1 : 0);
    }

    if (bVerAbsGr0)
    {
        if (uiVerAbs > 1)
        {
            xWriteEpExGolomb(uiVerAbs - 2, 1);
        }

        m_pcBinIf->encodeBinEP(0 > iVer ? 1 : 0);
    }
}

void TEncSbac::codeDeltaQP(TComDataCU* cu, UInt absPartIdx)
{
    int iDQp  = cu->getQP(absPartIdx) - cu->getRefQP(absPartIdx);

    int qpBdOffsetY =  cu->getSlice()->getSPS()->getQpBDOffsetY();

    iDQp = (iDQp + 78 + qpBdOffsetY + (qpBdOffsetY / 2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY / 2);

    UInt uiAbsDQp = (UInt)((iDQp > 0) ? iDQp  : (-iDQp));
    UInt TUValue = X265_MIN((int)uiAbsDQp, CU_DQP_TU_CMAX);
    xWriteUnaryMaxSymbol(TUValue, &m_cCUDeltaQpSCModel.get(0), 1, CU_DQP_TU_CMAX);
    if (uiAbsDQp >= CU_DQP_TU_CMAX)
    {
        xWriteEpExGolomb(uiAbsDQp - CU_DQP_TU_CMAX, CU_DQP_EG_k);
    }

    if (uiAbsDQp > 0)
    {
        UInt uiSign = (iDQp > 0 ? 0 : 1);
        m_pcBinIf->encodeBinEP(uiSign);
    }
}

void TEncSbac::codeQtCbf(TComDataCU* cu, UInt absPartIdx, TextType ttype, UInt trDepth)
{
    UInt uiCbf = cu->getCbf(absPartIdx, ttype, trDepth);
    UInt uiCtx = cu->getCtxQtCbf(ttype, trDepth);

    m_pcBinIf->encodeBin(uiCbf, m_cCUQtCbfSCModel.get((ttype ? TEXT_CHROMA : 0) * NUM_QT_CBF_CTX + uiCtx));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseQtCbf()")
    DTRACE_CABAC_T("\tsymbol=")
    DTRACE_CABAC_V(uiCbf)
    DTRACE_CABAC_T("\tctx=")
    DTRACE_CABAC_V(uiCtx)
    DTRACE_CABAC_T("\tetype=")
    DTRACE_CABAC_V(ttype)
    DTRACE_CABAC_T("\tuiAbsPartIdx=")
    DTRACE_CABAC_V(absPartIdx)
    DTRACE_CABAC_T("\n")
}

void TEncSbac::codeTransformSkipFlags(TComDataCU* cu, UInt absPartIdx, UInt width, UInt height, TextType eTType)
{
    if (cu->getCUTransquantBypass(absPartIdx))
    {
        return;
    }
    if (width != 4 || height != 4)
    {
        return;
    }

    UInt useTransformSkip = cu->getTransformSkip(absPartIdx, eTType);
    m_pcBinIf->encodeBin(useTransformSkip, m_cTransformSkipSCModel.get((eTType ? TEXT_CHROMA : TEXT_LUMA) * NUM_TRANSFORMSKIP_FLAG_CTX));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseTransformSkip()");
    DTRACE_CABAC_T("\tsymbol=")
    DTRACE_CABAC_V(useTransformSkip)
    DTRACE_CABAC_T("\tAddr=")
    DTRACE_CABAC_V(cu->getAddr())
    DTRACE_CABAC_T("\tetype=")
    DTRACE_CABAC_V(eTType)
    DTRACE_CABAC_T("\tuiAbsPartIdx=")
    DTRACE_CABAC_V(absPartIdx)
    DTRACE_CABAC_T("\n")
}

/** Code I_PCM information.
 * \param cu pointer to CU
 * \param absPartIdx CU index
 * \returns void
 */
void TEncSbac::codeIPCMInfo(TComDataCU* cu, UInt absPartIdx)
{
    UInt uiIPCM = (cu->getIPCMFlag(absPartIdx) == true) ? 1 : 0;

    bool writePCMSampleFlag = cu->getIPCMFlag(absPartIdx);

    m_pcBinIf->encodeBinTrm(uiIPCM);

    if (writePCMSampleFlag)
    {
        m_pcBinIf->encodePCMAlignBits();

        UInt uiMinCoeffSize = cu->getPic()->getMinCUWidth() * cu->getPic()->getMinCUHeight();
        UInt uiLumaOffset   = uiMinCoeffSize * absPartIdx;
        UInt uiChromaOffset = uiLumaOffset >> 2;
        Pel* piPCMSample;
        UInt width;
        UInt height;
        UInt uiSampleBits;
        UInt uiX, uiY;

        piPCMSample = cu->getPCMSampleY() + uiLumaOffset;
        width = cu->getWidth(absPartIdx);
        height = cu->getHeight(absPartIdx);
        uiSampleBits = cu->getSlice()->getSPS()->getPCMBitDepthLuma();

        for (uiY = 0; uiY < height; uiY++)
        {
            for (uiX = 0; uiX < width; uiX++)
            {
                UInt uiSample = piPCMSample[uiX];

                m_pcBinIf->xWritePCMCode(uiSample, uiSampleBits);
            }

            piPCMSample += width;
        }

        piPCMSample = cu->getPCMSampleCb() + uiChromaOffset;
        width = cu->getWidth(absPartIdx) / 2;
        height = cu->getHeight(absPartIdx) / 2;
        uiSampleBits = cu->getSlice()->getSPS()->getPCMBitDepthChroma();

        for (uiY = 0; uiY < height; uiY++)
        {
            for (uiX = 0; uiX < width; uiX++)
            {
                UInt uiSample = piPCMSample[uiX];

                m_pcBinIf->xWritePCMCode(uiSample, uiSampleBits);
            }

            piPCMSample += width;
        }

        piPCMSample = cu->getPCMSampleCr() + uiChromaOffset;
        width = cu->getWidth(absPartIdx) / 2;
        height = cu->getHeight(absPartIdx) / 2;
        uiSampleBits = cu->getSlice()->getSPS()->getPCMBitDepthChroma();

        for (uiY = 0; uiY < height; uiY++)
        {
            for (uiX = 0; uiX < width; uiX++)
            {
                UInt uiSample = piPCMSample[uiX];

                m_pcBinIf->xWritePCMCode(uiSample, uiSampleBits);
            }

            piPCMSample += width;
        }

        m_pcBinIf->resetBac();
    }
}

void TEncSbac::codeQtRootCbf(TComDataCU* cu, UInt absPartIdx)
{
    UInt uiCbf = cu->getQtRootCbf(absPartIdx);
    UInt uiCtx = 0;

    m_pcBinIf->encodeBin(uiCbf, m_cCUQtRootCbfSCModel.get(uiCtx));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseQtRootCbf()")
    DTRACE_CABAC_T("\tsymbol=")
    DTRACE_CABAC_V(uiCbf)
    DTRACE_CABAC_T("\tctx=")
    DTRACE_CABAC_V(uiCtx)
    DTRACE_CABAC_T("\tuiAbsPartIdx=")
    DTRACE_CABAC_V(absPartIdx)
    DTRACE_CABAC_T("\n")
}

void TEncSbac::codeQtCbfZero(TComDataCU* cu, TextType ttype, UInt trDepth)
{
    // this function is only used to estimate the bits when cbf is 0
    // and will never be called when writing the bistream. do not need to write log
    UInt cbf = 0;
    UInt ctx = cu->getCtxQtCbf(ttype, trDepth);

    m_pcBinIf->encodeBin(cbf, m_cCUQtCbfSCModel.get((ttype ? TEXT_CHROMA : 0) * NUM_QT_CBF_CTX + ctx));
}

void TEncSbac::codeQtRootCbfZero(TComDataCU*)
{
    // this function is only used to estimate the bits when cbf is 0
    // and will never be called when writing the bistream. do not need to write log
    UInt cbf = 0;
    UInt ctx = 0;

    m_pcBinIf->encodeBin(cbf, m_cCUQtRootCbfSCModel.get(ctx));
}

/** Encode (X,Y) position of the last significant coefficient
 * \param posx X component of last coefficient
 * \param posy Y component of last coefficient
 * \param width  Block width
 * \param height Block height
 * \param eTType plane type / luminance or chrominance
 * \param uiScanIdx scan type (zig-zag, hor, ver)
 * This method encodes the X and Y component within a block of the last significant coefficient.
 */
void TEncSbac::codeLastSignificantXY(UInt posx, UInt posy, int width, int height, TextType eTType, UInt uiScanIdx)
{
    assert((eTType == TEXT_LUMA) || (eTType == TEXT_CHROMA));
    // swap
    if (uiScanIdx == SCAN_VER)
    {
        std::swap(posx, posy);
    }

    UInt uiCtxLast;
    ContextModel *pCtxX = &m_cCuCtxLastX.get(eTType ? NUM_CTX_LAST_FLAG_XY : 0);
    ContextModel *pCtxY = &m_cCuCtxLastY.get(eTType ? NUM_CTX_LAST_FLAG_XY : 0);
    UInt uiGroupIdxX    = g_groupIdx[posx];
    UInt uiGroupIdxY    = g_groupIdx[posy];

    int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;
    blkSizeOffsetX = eTType ? 0 : (g_convertToBit[width] * 3 + ((g_convertToBit[width] + 1) >> 2));
    blkSizeOffsetY = eTType ? 0 : (g_convertToBit[height] * 3 + ((g_convertToBit[height] + 1) >> 2));
    shiftX = eTType ? g_convertToBit[width] : ((g_convertToBit[width] + 3) >> 2);
    shiftY = eTType ? g_convertToBit[height] : ((g_convertToBit[height] + 3) >> 2);
    // posX
    for (uiCtxLast = 0; uiCtxLast < uiGroupIdxX; uiCtxLast++)
    {
        m_pcBinIf->encodeBin(1, *(pCtxX + blkSizeOffsetX + (uiCtxLast >> shiftX)));
    }

    if (uiGroupIdxX < g_groupIdx[width - 1])
    {
        m_pcBinIf->encodeBin(0, *(pCtxX + blkSizeOffsetX + (uiCtxLast >> shiftX)));
    }

    // posY
    for (uiCtxLast = 0; uiCtxLast < uiGroupIdxY; uiCtxLast++)
    {
        m_pcBinIf->encodeBin(1, *(pCtxY + blkSizeOffsetY + (uiCtxLast >> shiftY)));
    }

    if (uiGroupIdxY < g_groupIdx[height - 1])
    {
        m_pcBinIf->encodeBin(0, *(pCtxY + blkSizeOffsetY + (uiCtxLast >> shiftY)));
    }
    if (uiGroupIdxX > 3)
    {
        UInt uiCount = (uiGroupIdxX - 2) >> 1;
        posx       = posx - g_minInGroup[uiGroupIdxX];
        m_pcBinIf->encodeBinsEP(posx, uiCount);
    }
    if (uiGroupIdxY > 3)
    {
        UInt uiCount = (uiGroupIdxY - 2) >> 1;
        posy       = posy - g_minInGroup[uiGroupIdxY];
        m_pcBinIf->encodeBinsEP(posy, uiCount);
    }
}

void TEncSbac::codeCoeffNxN(TComDataCU* cu, TCoeff* pcCoef, UInt absPartIdx, UInt width, UInt height, UInt depth, TextType eTType)
{
#if ENC_DEC_TRACE
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseCoeffNxN()\teType=")
    DTRACE_CABAC_V(eTType)
    DTRACE_CABAC_T("\twidth=")
    DTRACE_CABAC_V(width)
    DTRACE_CABAC_T("\theight=")
    DTRACE_CABAC_V(height)
    DTRACE_CABAC_T("\tdepth=")
    DTRACE_CABAC_V(depth)
    DTRACE_CABAC_T("\tabspartidx=")
    DTRACE_CABAC_V(absPartIdx)
    DTRACE_CABAC_T("\ttoCU-X=")
    DTRACE_CABAC_V(cu->getCUPelX())
    DTRACE_CABAC_T("\ttoCU-Y=")
    DTRACE_CABAC_V(cu->getCUPelY())
    DTRACE_CABAC_T("\tCU-addr=")
    DTRACE_CABAC_V(cu->getAddr())
    DTRACE_CABAC_T("\tinCU-X=")
    DTRACE_CABAC_V(g_rasterToPelX[g_zscanToRaster[absPartIdx]])
    DTRACE_CABAC_T("\tinCU-Y=")
    DTRACE_CABAC_V(g_rasterToPelY[g_zscanToRaster[absPartIdx]])
    DTRACE_CABAC_T("\tpredmode=")
    DTRACE_CABAC_V(cu->getPredictionMode(absPartIdx))
    DTRACE_CABAC_T("\n")
#else
    (void)depth;
#endif

    if (width > m_pcSlice->getSPS()->getMaxTrSize())
    {
        width  = m_pcSlice->getSPS()->getMaxTrSize();
        height = m_pcSlice->getSPS()->getMaxTrSize();
    }

    UInt uiNumSig = 0;

    // compute number of significant coefficients
    uiNumSig = TEncEntropy::countNonZeroCoeffs(pcCoef, width * height);

    if (uiNumSig == 0)
        return;
    if (cu->getSlice()->getPPS()->getUseTransformSkip())
    {
        codeTransformSkipFlags(cu, absPartIdx, width, height, eTType);
    }
    eTType = eTType == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA ;

    //----- encode significance map -----
    const UInt   uiLog2BlockSize = g_convertToBit[width] + 2;
    UInt uiScanIdx = cu->getCoefScanIdx(absPartIdx, width, eTType == TEXT_LUMA, cu->isIntra(absPartIdx));
    const UInt *scan = g_sigLastScan[uiScanIdx][uiLog2BlockSize - 1];

    bool beValid;
    if (cu->getCUTransquantBypass(absPartIdx))
    {
        beValid = false;
    }
    else
    {
        beValid = cu->getSlice()->getPPS()->getSignHideFlag() > 0;
    }

    // Find position of last coefficient
    int scanPosLast = -1;
    int posLast;

    const UInt * scanCG;
    {
        scanCG = g_sigLastScan[uiScanIdx][uiLog2BlockSize > 3 ? uiLog2BlockSize - 2 - 1 : 0];
        if (uiLog2BlockSize == 3)
        {
            scanCG = g_sigLastScan8x8[uiScanIdx];
        }
        else if (uiLog2BlockSize == 5)
        {
            scanCG = g_sigLastScanCG32x32;
        }
    }
    UInt uiSigCoeffGroupFlag[MLS_GRP_NUM];
    static const UInt uiShift = MLS_CG_SIZE >> 1;
    const UInt uiNumBlkSide = width >> uiShift;

    ::memset(uiSigCoeffGroupFlag, 0, sizeof(UInt) * MLS_GRP_NUM);

    do
    {
        posLast = scan[++scanPosLast];

        // get L1 sig map
        UInt posy    = posLast >> uiLog2BlockSize;
        UInt posx    = posLast - (posy << uiLog2BlockSize);
        UInt uiBlkIdx  = uiNumBlkSide * (posy >> uiShift) + (posx >> uiShift);
        if (pcCoef[posLast])
        {
            uiSigCoeffGroupFlag[uiBlkIdx] = 1;
        }

        uiNumSig -= (pcCoef[posLast] != 0);
    }
    while (uiNumSig > 0);

    // Code position of last coefficient
    int posLastY = posLast >> uiLog2BlockSize;
    int posLastX = posLast - (posLastY << uiLog2BlockSize);
    codeLastSignificantXY(posLastX, posLastY, width, height, eTType, uiScanIdx);

    //===== code significance flag =====
    ContextModel * const baseCoeffGroupCtx = &m_cCUSigCoeffGroupSCModel.get(eTType ? NUM_SIG_CG_FLAG_CTX : 0);
    ContextModel * const baseCtx = (eTType == TEXT_LUMA) ? &m_cCUSigSCModel.get(0) : &m_cCUSigSCModel.get(NUM_SIG_FLAG_CTX_LUMA);

    const int  iLastScanSet      = scanPosLast >> LOG2_SCAN_SET_SIZE;
    UInt c1 = 1;
    UInt uiGoRiceParam           = 0;
    int  iScanPosSig             = scanPosLast;

    for (int iSubSet = iLastScanSet; iSubSet >= 0; iSubSet--)
    {
        int numNonZero = 0;
        int  iSubPos     = iSubSet << LOG2_SCAN_SET_SIZE;
        uiGoRiceParam    = 0;
        int absCoeff[16];
        UInt coeffSigns = 0;

        int lastNZPosInCG = -1, firstNZPosInCG = SCAN_SET_SIZE;

        if (iScanPosSig == scanPosLast)
        {
            absCoeff[0] = abs(pcCoef[posLast]);
            coeffSigns    = (pcCoef[posLast] < 0);
            numNonZero    = 1;
            lastNZPosInCG  = iScanPosSig;
            firstNZPosInCG = iScanPosSig;
            iScanPosSig--;
        }

        // encode significant_coeffgroup_flag
        int iCGBlkPos = scanCG[iSubSet];
        int iCGPosY   = iCGBlkPos / uiNumBlkSide;
        int iCGPosX   = iCGBlkPos - (iCGPosY * uiNumBlkSide);
        if (iSubSet == iLastScanSet || iSubSet == 0)
        {
            uiSigCoeffGroupFlag[iCGBlkPos] = 1;
        }
        else
        {
            UInt uiSigCoeffGroup   = (uiSigCoeffGroupFlag[iCGBlkPos] != 0);
            UInt uiCtxSig  = TComTrQuant::getSigCoeffGroupCtxInc(uiSigCoeffGroupFlag, iCGPosX, iCGPosY, width, height);
            m_pcBinIf->encodeBin(uiSigCoeffGroup, baseCoeffGroupCtx[uiCtxSig]);
        }

        // encode significant_coeff_flag
        if (uiSigCoeffGroupFlag[iCGBlkPos])
        {
            int patternSigCtx = TComTrQuant::calcPatternSigCtx(uiSigCoeffGroupFlag, iCGPosX, iCGPosY, width, height);
            UInt uiBlkPos, posy, posx, uiSig, uiCtxSig;
            for (; iScanPosSig >= iSubPos; iScanPosSig--)
            {
                uiBlkPos  = scan[iScanPosSig];
                posy    = uiBlkPos >> uiLog2BlockSize;
                posx    = uiBlkPos - (posy << uiLog2BlockSize);
                uiSig     = (pcCoef[uiBlkPos] != 0);
                if (iScanPosSig > iSubPos || iSubSet == 0 || numNonZero)
                {
                    uiCtxSig  = TComTrQuant::getSigCtxInc(patternSigCtx, uiScanIdx, posx, posy, uiLog2BlockSize, eTType);
                    m_pcBinIf->encodeBin(uiSig, baseCtx[uiCtxSig]);
                }
                if (uiSig)
                {
                    absCoeff[numNonZero] = abs(pcCoef[uiBlkPos]);
                    coeffSigns = 2 * coeffSigns + (pcCoef[uiBlkPos] < 0);
                    numNonZero++;
                    if (lastNZPosInCG == -1)
                    {
                        lastNZPosInCG = iScanPosSig;
                    }
                    firstNZPosInCG = iScanPosSig;
                }
            }
        }
        else
        {
            iScanPosSig = iSubPos - 1;
        }

        if (numNonZero > 0)
        {
            bool signHidden = (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD);
            UInt uiCtxSet = (iSubSet > 0 && eTType == TEXT_LUMA) ? 2 : 0;

            if (c1 == 0)
            {
                uiCtxSet++;
            }
            c1 = 1;
            ContextModel *baseCtxMod = (eTType == TEXT_LUMA) ? &m_cCUOneSCModel.get(4 * uiCtxSet) : &m_cCUOneSCModel.get(NUM_ONE_FLAG_CTX_LUMA + 4 * uiCtxSet);

            int numC1Flag = X265_MIN(numNonZero, C1FLAG_NUMBER);
            int firstC2FlagIdx = -1;
            for (int idx = 0; idx < numC1Flag; idx++)
            {
                UInt uiSymbol = absCoeff[idx] > 1;
                m_pcBinIf->encodeBin(uiSymbol, baseCtxMod[c1]);
                if (uiSymbol)
                {
                    c1 = 0;

                    if (firstC2FlagIdx == -1)
                    {
                        firstC2FlagIdx = idx;
                    }
                }
                else if ((c1 < 3) && (c1 > 0))
                {
                    c1++;
                }
            }

            if (c1 == 0)
            {
                baseCtxMod = (eTType == TEXT_LUMA) ? &m_cCUAbsSCModel.get(uiCtxSet) : &m_cCUAbsSCModel.get(NUM_ABS_FLAG_CTX_LUMA + uiCtxSet);
                if (firstC2FlagIdx != -1)
                {
                    UInt symbol = absCoeff[firstC2FlagIdx] > 2;
                    m_pcBinIf->encodeBin(symbol, baseCtxMod[0]);
                }
            }

            if (beValid && signHidden)
            {
                m_pcBinIf->encodeBinsEP((coeffSigns >> 1), numNonZero - 1);
            }
            else
            {
                m_pcBinIf->encodeBinsEP(coeffSigns, numNonZero);
            }

            int iFirstCoeff2 = 1;
            if (c1 == 0 || numNonZero > C1FLAG_NUMBER)
            {
                for (int idx = 0; idx < numNonZero; idx++)
                {
                    UInt baseLevel  = (idx < C1FLAG_NUMBER) ? (2 + iFirstCoeff2) : 1;

                    if (absCoeff[idx] >= baseLevel)
                    {
                        xWriteCoefRemainExGolomb(absCoeff[idx] - baseLevel, uiGoRiceParam);
                        if (absCoeff[idx] > 3 * (1 << uiGoRiceParam))
                        {
                            uiGoRiceParam = std::min<UInt>(uiGoRiceParam + 1, 4);
                        }
                    }
                    if (absCoeff[idx] >= 2)
                    {
                        iFirstCoeff2 = 0;
                    }
                }
            }
        }
    }
}

/** code SAO offset sign
 * \param code sign value
 */
void TEncSbac::codeSAOSign(UInt code)
{
    m_pcBinIf->encodeBinEP(code);
}

void TEncSbac::codeSaoMaxUvlc(UInt code, UInt maxSymbol)
{
    if (maxSymbol == 0)
    {
        return;
    }

    int i;
    bool bCodeLast = (maxSymbol > code);

    if (code == 0)
    {
        m_pcBinIf->encodeBinEP(0);
    }
    else
    {
        m_pcBinIf->encodeBinEP(1);
        for (i = 0; i < code - 1; i++)
        {
            m_pcBinIf->encodeBinEP(1);
        }

        if (bCodeLast)
        {
            m_pcBinIf->encodeBinEP(0);
        }
    }
}

/** Code SAO EO class or BO band position
 * \param uiLength
 * \param uiCode
 */
void TEncSbac::codeSaoUflc(UInt uiLength, UInt uiCode)
{
    m_pcBinIf->encodeBinsEP(uiCode, uiLength);
}

/** Code SAO merge flags
 * \param uiCode
 * \param uiCompIdx
 */
void TEncSbac::codeSaoMerge(UInt uiCode)
{
    if (uiCode == 0)
    {
        m_pcBinIf->encodeBin(0,  m_cSaoMergeSCModel.get(0));
    }
    else
    {
        m_pcBinIf->encodeBin(1,  m_cSaoMergeSCModel.get(0));
    }
}

/** Code SAO type index
 * \param uiCode
 */
void TEncSbac::codeSaoTypeIdx(UInt uiCode)
{
    if (uiCode == 0)
    {
        m_pcBinIf->encodeBin(0, m_cSaoTypeIdxSCModel.get(0));
    }
    else
    {
        m_pcBinIf->encodeBin(1, m_cSaoTypeIdxSCModel.get(0));
        m_pcBinIf->encodeBinEP(uiCode <= 4 ? 1 : 0);
    }
}

/*!
 ****************************************************************************
 * \brief
 *   estimate bit cost for CBP, significant map and significant coefficients
 ****************************************************************************
 */
void TEncSbac::estBit(estBitsSbacStruct* pcEstBitsSbac, int width, int height, TextType eTType)
{
    estCBFBit(pcEstBitsSbac);

    estSignificantCoeffGroupMapBit(pcEstBitsSbac, eTType);

    // encode significance map
    estSignificantMapBit(pcEstBitsSbac, width, height, eTType);

    // encode significant coefficients
    estSignificantCoefficientsBit(pcEstBitsSbac, eTType);
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost for each CBP bit
 ****************************************************************************
 */
void TEncSbac::estCBFBit(estBitsSbacStruct* pcEstBitsSbac)
{
    ContextModel *pCtx = &m_cCUQtCbfSCModel.get(0);

    for (UInt uiCtxInc = 0; uiCtxInc < 3 * NUM_QT_CBF_CTX; uiCtxInc++)
    {
        pcEstBitsSbac->blockCbpBits[uiCtxInc][0] = pCtx[uiCtxInc].getEntropyBits(0);
        pcEstBitsSbac->blockCbpBits[uiCtxInc][1] = pCtx[uiCtxInc].getEntropyBits(1);
    }

    pCtx = &m_cCUQtRootCbfSCModel.get(0);

    for (UInt uiCtxInc = 0; uiCtxInc < 4; uiCtxInc++)
    {
        pcEstBitsSbac->blockRootCbpBits[uiCtxInc][0] = pCtx[uiCtxInc].getEntropyBits(0);
        pcEstBitsSbac->blockRootCbpBits[uiCtxInc][1] = pCtx[uiCtxInc].getEntropyBits(1);
    }
}

/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient group map
 ****************************************************************************
 */
void TEncSbac::estSignificantCoeffGroupMapBit(estBitsSbacStruct* pcEstBitsSbac, TextType eTType)
{
    assert((eTType == TEXT_LUMA) || (eTType == TEXT_CHROMA));
    int firstCtx = 0, numCtx = NUM_SIG_CG_FLAG_CTX;

    for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
    {
        for (UInt uiBin = 0; uiBin < 2; uiBin++)
        {
            pcEstBitsSbac->significantCoeffGroupBits[ctxIdx][uiBin] = m_cCUSigCoeffGroupSCModel.get((eTType ? NUM_SIG_CG_FLAG_CTX : 0) + ctxIdx).getEntropyBits(uiBin);
        }
    }
}

/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient map
 ****************************************************************************
 */
void TEncSbac::estSignificantMapBit(estBitsSbacStruct* pcEstBitsSbac, int width, int height, TextType eTType)
{
    int firstCtx = 1, numCtx = 8;

    if (X265_MAX(width, height) >= 16)
    {
        firstCtx = (eTType == TEXT_LUMA) ? 21 : 12;
        numCtx = (eTType == TEXT_LUMA) ? 6 : 3;
    }
    else if (width == 8)
    {
        firstCtx = 9;
        numCtx = (eTType == TEXT_LUMA) ? 12 : 3;
    }

    if (eTType == TEXT_LUMA)
    {
        for (UInt bin = 0; bin < 2; bin++)
        {
            pcEstBitsSbac->significantBits[0][bin] = m_cCUSigSCModel.get(0).getEntropyBits(bin);
        }

        for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
        {
            for (UInt uiBin = 0; uiBin < 2; uiBin++)
            {
                pcEstBitsSbac->significantBits[ctxIdx][uiBin] = m_cCUSigSCModel.get(ctxIdx).getEntropyBits(uiBin);
            }
        }
    }
    else
    {
        for (UInt bin = 0; bin < 2; bin++)
        {
            pcEstBitsSbac->significantBits[0][bin] = m_cCUSigSCModel.get(NUM_SIG_FLAG_CTX_LUMA + 0).getEntropyBits(bin);
        }

        for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
        {
            for (UInt uiBin = 0; uiBin < 2; uiBin++)
            {
                pcEstBitsSbac->significantBits[ctxIdx][uiBin] = m_cCUSigSCModel.get(NUM_SIG_FLAG_CTX_LUMA + ctxIdx).getEntropyBits(uiBin);
            }
        }
    }
    int iBitsX = 0, iBitsY = 0;
    int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;

    blkSizeOffsetX = eTType ? 0 : (g_convertToBit[width] * 3 + ((g_convertToBit[width] + 1) >> 2));
    blkSizeOffsetY = eTType ? 0 : (g_convertToBit[height] * 3 + ((g_convertToBit[height] + 1) >> 2));
    shiftX = eTType ? g_convertToBit[width] : ((g_convertToBit[width] + 3) >> 2);
    shiftY = eTType ? g_convertToBit[height] : ((g_convertToBit[height] + 3) >> 2);

    assert((eTType == TEXT_LUMA) || (eTType == TEXT_CHROMA));
    int ctx;
    for (ctx = 0; ctx < g_groupIdx[width - 1]; ctx++)
    {
        int ctxOffset = blkSizeOffsetX + (ctx >> shiftX);
        pcEstBitsSbac->lastXBits[ctx] = iBitsX + m_cCuCtxLastX.get((eTType ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset).getEntropyBits(0);
        iBitsX += m_cCuCtxLastX.get((eTType ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset).getEntropyBits(1);
    }

    pcEstBitsSbac->lastXBits[ctx] = iBitsX;
    for (ctx = 0; ctx < g_groupIdx[height - 1]; ctx++)
    {
        int ctxOffset = blkSizeOffsetY + (ctx >> shiftY);
        pcEstBitsSbac->lastYBits[ctx] = iBitsY + m_cCuCtxLastY.get((eTType ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset).getEntropyBits(0);
        iBitsY += m_cCuCtxLastY.get((eTType ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset).getEntropyBits(1);
    }

    pcEstBitsSbac->lastYBits[ctx] = iBitsY;
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost of significant coefficient
 ****************************************************************************
 */
void TEncSbac::estSignificantCoefficientsBit(estBitsSbacStruct* pcEstBitsSbac, TextType eTType)
{
    if (eTType == TEXT_LUMA)
    {
        ContextModel *ctxOne = &m_cCUOneSCModel.get(0);
        ContextModel *ctxAbs = &m_cCUAbsSCModel.get(0);

        for (int ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_LUMA; ctxIdx++)
        {
            pcEstBitsSbac->greaterOneBits[ctxIdx][0] = ctxOne[ctxIdx].getEntropyBits(0);
            pcEstBitsSbac->greaterOneBits[ctxIdx][1] = ctxOne[ctxIdx].getEntropyBits(1);
        }

        for (int ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_LUMA; ctxIdx++)
        {
            pcEstBitsSbac->levelAbsBits[ctxIdx][0] = ctxAbs[ctxIdx].getEntropyBits(0);
            pcEstBitsSbac->levelAbsBits[ctxIdx][1] = ctxAbs[ctxIdx].getEntropyBits(1);
        }
    }
    else
    {
        ContextModel *ctxOne = &m_cCUOneSCModel.get(NUM_ONE_FLAG_CTX_LUMA);
        ContextModel *ctxAbs = &m_cCUAbsSCModel.get(NUM_ABS_FLAG_CTX_LUMA);

        for (int ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_CHROMA; ctxIdx++)
        {
            pcEstBitsSbac->greaterOneBits[ctxIdx][0] = ctxOne[ctxIdx].getEntropyBits(0);
            pcEstBitsSbac->greaterOneBits[ctxIdx][1] = ctxOne[ctxIdx].getEntropyBits(1);
        }

        for (int ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_CHROMA; ctxIdx++)
        {
            pcEstBitsSbac->levelAbsBits[ctxIdx][0] = ctxAbs[ctxIdx].getEntropyBits(0);
            pcEstBitsSbac->levelAbsBits[ctxIdx][1] = ctxAbs[ctxIdx].getEntropyBits(1);
        }
    }
}

/**
 - Initialize our context information from the nominated source.
 .
 \param src From where to copy context information.
 */
void TEncSbac::xCopyContextsFrom(TEncSbac* src)
{
    memcpy(m_contextModels, src->m_contextModels, m_numContextModels * sizeof(m_contextModels[0]));
}

void  TEncSbac::loadContexts(TEncSbac* pScr)
{
    this->xCopyContextsFrom(pScr);
}

//! \}
