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
    : m_bitIf(NULL)
    , m_slice(NULL)
    , m_binIf(NULL)
    , m_coeffCost(0)
    , m_numContextModels(0)
    , m_cuSplitFlagSCModel(1,          NUM_SPLIT_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuSkipFlagSCModel(1,           NUM_SKIP_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuMergeFlagExtSCModel(1,       NUM_MERGE_FLAG_EXT_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuMergeIdxExtSCModel(1,        NUM_MERGE_IDX_EXT_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuPartSizeSCModel(1,           NUM_PART_SIZE_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuPredModeSCModel(1,           NUM_PRED_MODE_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuIntraPredSCModel(1,          NUM_ADI_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuChromaPredSCModel(1,         NUM_CHROMA_PRED_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuDeltaQpSCModel(1,            NUM_DELTA_QP_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuInterDirSCModel(1,           NUM_INTER_DIR_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuRefPicSCModel(1,             NUM_REF_NO_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuMvdSCModel(1,                NUM_MV_RES_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuQtCbfSCModel(2,              NUM_QT_CBF_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuTransSubdivFlagSCModel(1,    NUM_TRANS_SUBDIV_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuQtRootCbfSCModel(1,          NUM_QT_ROOT_CBF_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuSigCoeffGroupSCModel(2,      NUM_SIG_CG_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuSigSCModel(1,                NUM_SIG_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuCtxLastX(2,                  NUM_CTX_LAST_FLAG_XY, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuCtxLastY(2,                  NUM_CTX_LAST_FLAG_XY, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuOneSCModel(1,                NUM_ONE_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuAbsSCModel(1,                NUM_ABS_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_mvpIdxSCModel(1,               NUM_MVP_IDX_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuAMPSCModel(1,                NUM_CU_AMP_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_saoMergeSCModel(1,             NUM_SAO_MERGE_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_saoTypeIdxSCModel(1,           NUM_SAO_TYPE_IDX_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_transformSkipSCModel(2,        NUM_TRANSFORMSKIP_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
    , m_cuTransquantBypassFlagSCModel(1,NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
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
    int  qp              = m_slice->getSliceQp();
    SliceType sliceType  = m_slice->getSliceType();

    int encCABACTableIdx = m_slice->getPPS()->getEncCABACTableIdx();

    if (!m_slice->isIntra() && (encCABACTableIdx == B_SLICE || encCABACTableIdx == P_SLICE) && m_slice->getPPS()->getCabacInitPresentFlag())
    {
        sliceType = (SliceType)encCABACTableIdx;
    }

    m_cuSplitFlagSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SPLIT_FLAG);

    m_cuSkipFlagSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SKIP_FLAG);
    m_cuMergeFlagExtSCModel.initBuffer(sliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT);
    m_cuMergeIdxExtSCModel.initBuffer(sliceType, qp, (UChar*)INIT_MERGE_IDX_EXT);
    m_cuPartSizeSCModel.initBuffer(sliceType, qp, (UChar*)INIT_PART_SIZE);
    m_cuAMPSCModel.initBuffer(sliceType, qp, (UChar*)INIT_CU_AMP_POS);
    m_cuPredModeSCModel.initBuffer(sliceType, qp, (UChar*)INIT_PRED_MODE);
    m_cuIntraPredSCModel.initBuffer(sliceType, qp, (UChar*)INIT_INTRA_PRED_MODE);
    m_cuChromaPredSCModel.initBuffer(sliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE);
    m_cuInterDirSCModel.initBuffer(sliceType, qp, (UChar*)INIT_INTER_DIR);
    m_cuMvdSCModel.initBuffer(sliceType, qp, (UChar*)INIT_MVD);
    m_cuRefPicSCModel.initBuffer(sliceType, qp, (UChar*)INIT_REF_PIC);
    m_cuDeltaQpSCModel.initBuffer(sliceType, qp, (UChar*)INIT_DQP);
    m_cuQtCbfSCModel.initBuffer(sliceType, qp, (UChar*)INIT_QT_CBF);
    m_cuQtRootCbfSCModel.initBuffer(sliceType, qp, (UChar*)INIT_QT_ROOT_CBF);
    m_cuSigCoeffGroupSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SIG_CG_FLAG);
    m_cuSigSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SIG_FLAG);
    m_cuCtxLastX.initBuffer(sliceType, qp, (UChar*)INIT_LAST);
    m_cuCtxLastY.initBuffer(sliceType, qp, (UChar*)INIT_LAST);
    m_cuOneSCModel.initBuffer(sliceType, qp, (UChar*)INIT_ONE_FLAG);
    m_cuAbsSCModel.initBuffer(sliceType, qp, (UChar*)INIT_ABS_FLAG);
    m_mvpIdxSCModel.initBuffer(sliceType, qp, (UChar*)INIT_MVP_IDX);
    m_cuTransSubdivFlagSCModel.initBuffer(sliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG);
    m_saoMergeSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG);
    m_saoTypeIdxSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SAO_TYPE_IDX);
    m_transformSkipSCModel.initBuffer(sliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG);
    m_cuTransquantBypassFlagSCModel.initBuffer(sliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG);
    // new structure
    m_lastQp = qp;

    m_binIf->start();
}

/** The function does the following:
 * If current slice type is P/B then it determines the distance of initialisation type 1 and 2 from the current CABAC states and
 * stores the index of the closest table.  This index is used for the next P/B slice when cabac_init_present_flag is true.
 */
void TEncSbac::determineCabacInitIdx()
{
    int qp = m_slice->getSliceQp();

    if (!m_slice->isIntra())
    {
        SliceType aSliceTypeChoices[] = { B_SLICE, P_SLICE };

        UInt bestCost             = MAX_UINT;
        SliceType bestSliceType   = aSliceTypeChoices[0];
        for (UInt idx = 0; idx < 2; idx++)
        {
            UInt curCost = 0;
            SliceType curSliceType = aSliceTypeChoices[idx];

            curCost  = m_cuSplitFlagSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SPLIT_FLAG);
            curCost += m_cuSkipFlagSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SKIP_FLAG);
            curCost += m_cuMergeFlagExtSCModel.calcCost(curSliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT);
            curCost += m_cuMergeIdxExtSCModel.calcCost(curSliceType, qp, (UChar*)INIT_MERGE_IDX_EXT);
            curCost += m_cuPartSizeSCModel.calcCost(curSliceType, qp, (UChar*)INIT_PART_SIZE);
            curCost += m_cuAMPSCModel.calcCost(curSliceType, qp, (UChar*)INIT_CU_AMP_POS);
            curCost += m_cuPredModeSCModel.calcCost(curSliceType, qp, (UChar*)INIT_PRED_MODE);
            curCost += m_cuIntraPredSCModel.calcCost(curSliceType, qp, (UChar*)INIT_INTRA_PRED_MODE);
            curCost += m_cuChromaPredSCModel.calcCost(curSliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE);
            curCost += m_cuInterDirSCModel.calcCost(curSliceType, qp, (UChar*)INIT_INTER_DIR);
            curCost += m_cuMvdSCModel.calcCost(curSliceType, qp, (UChar*)INIT_MVD);
            curCost += m_cuRefPicSCModel.calcCost(curSliceType, qp, (UChar*)INIT_REF_PIC);
            curCost += m_cuDeltaQpSCModel.calcCost(curSliceType, qp, (UChar*)INIT_DQP);
            curCost += m_cuQtCbfSCModel.calcCost(curSliceType, qp, (UChar*)INIT_QT_CBF);
            curCost += m_cuQtRootCbfSCModel.calcCost(curSliceType, qp, (UChar*)INIT_QT_ROOT_CBF);
            curCost += m_cuSigCoeffGroupSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SIG_CG_FLAG);
            curCost += m_cuSigSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SIG_FLAG);
            curCost += m_cuCtxLastX.calcCost(curSliceType, qp, (UChar*)INIT_LAST);
            curCost += m_cuCtxLastY.calcCost(curSliceType, qp, (UChar*)INIT_LAST);
            curCost += m_cuOneSCModel.calcCost(curSliceType, qp, (UChar*)INIT_ONE_FLAG);
            curCost += m_cuAbsSCModel.calcCost(curSliceType, qp, (UChar*)INIT_ABS_FLAG);
            curCost += m_mvpIdxSCModel.calcCost(curSliceType, qp, (UChar*)INIT_MVP_IDX);
            curCost += m_cuTransSubdivFlagSCModel.calcCost(curSliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG);
            curCost += m_saoMergeSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG);
            curCost += m_saoTypeIdxSCModel.calcCost(curSliceType, qp, (UChar*)INIT_SAO_TYPE_IDX);
            curCost += m_transformSkipSCModel.calcCost(curSliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG);
            curCost += m_cuTransquantBypassFlagSCModel.calcCost(curSliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG);
            if (curCost < bestCost)
            {
                bestSliceType = curSliceType;
                bestCost      = curCost;
            }
        }

        m_slice->getPPS()->setEncCABACTableIdx(bestSliceType);
    }
    else
    {
        m_slice->getPPS()->setEncCABACTableIdx(I_SLICE);
    }
}

/** The function does the followng: Write out terminate bit. Flush CABAC. Intialize CABAC states. Start CABAC.
 */
void TEncSbac::updateContextTables(SliceType sliceType, int qp, bool bExecuteFinish)
{
    m_binIf->encodeBinTrm(1);
    if (bExecuteFinish) m_binIf->finish();
    m_cuSplitFlagSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SPLIT_FLAG);

    m_cuSkipFlagSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SKIP_FLAG);
    m_cuMergeFlagExtSCModel.initBuffer(sliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT);
    m_cuMergeIdxExtSCModel.initBuffer(sliceType, qp, (UChar*)INIT_MERGE_IDX_EXT);
    m_cuPartSizeSCModel.initBuffer(sliceType, qp, (UChar*)INIT_PART_SIZE);
    m_cuAMPSCModel.initBuffer(sliceType, qp, (UChar*)INIT_CU_AMP_POS);
    m_cuPredModeSCModel.initBuffer(sliceType, qp, (UChar*)INIT_PRED_MODE);
    m_cuIntraPredSCModel.initBuffer(sliceType, qp, (UChar*)INIT_INTRA_PRED_MODE);
    m_cuChromaPredSCModel.initBuffer(sliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE);
    m_cuInterDirSCModel.initBuffer(sliceType, qp, (UChar*)INIT_INTER_DIR);
    m_cuMvdSCModel.initBuffer(sliceType, qp, (UChar*)INIT_MVD);
    m_cuRefPicSCModel.initBuffer(sliceType, qp, (UChar*)INIT_REF_PIC);
    m_cuDeltaQpSCModel.initBuffer(sliceType, qp, (UChar*)INIT_DQP);
    m_cuQtCbfSCModel.initBuffer(sliceType, qp, (UChar*)INIT_QT_CBF);
    m_cuQtRootCbfSCModel.initBuffer(sliceType, qp, (UChar*)INIT_QT_ROOT_CBF);
    m_cuSigCoeffGroupSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SIG_CG_FLAG);
    m_cuSigSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SIG_FLAG);
    m_cuCtxLastX.initBuffer(sliceType, qp, (UChar*)INIT_LAST);
    m_cuCtxLastY.initBuffer(sliceType, qp, (UChar*)INIT_LAST);
    m_cuOneSCModel.initBuffer(sliceType, qp, (UChar*)INIT_ONE_FLAG);
    m_cuAbsSCModel.initBuffer(sliceType, qp, (UChar*)INIT_ABS_FLAG);
    m_mvpIdxSCModel.initBuffer(sliceType, qp, (UChar*)INIT_MVP_IDX);
    m_cuTransSubdivFlagSCModel.initBuffer(sliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG);
    m_saoMergeSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG);
    m_saoTypeIdxSCModel.initBuffer(sliceType, qp, (UChar*)INIT_SAO_TYPE_IDX);
    m_transformSkipSCModel.initBuffer(sliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG);
    m_cuTransquantBypassFlagSCModel.initBuffer(sliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG);
    m_binIf->start();
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

void TEncSbac::codeTerminatingBit(UInt lsLast)
{
    m_binIf->encodeBinTrm(lsLast);
}

void TEncSbac::codeSliceFinish()
{
    m_binIf->finish();
}

void TEncSbac::xWriteUnarySymbol(UInt symbol, ContextModel* scmModel, int offset)
{
    m_binIf->encodeBin(symbol ? 1 : 0, scmModel[0]);

    if (0 == symbol)
    {
        return;
    }

    while (symbol--)
    {
        m_binIf->encodeBin(symbol ? 1 : 0, scmModel[offset]);
    }
}

void TEncSbac::xWriteUnaryMaxSymbol(UInt symbol, ContextModel* scmModel, int offset, UInt maxSymbol)
{
    if (maxSymbol == 0)
    {
        return;
    }

    m_binIf->encodeBin(symbol ? 1 : 0, scmModel[0]);

    if (symbol == 0)
    {
        return;
    }

    bool bCodeLast = (maxSymbol > symbol);

    while (--symbol)
    {
        m_binIf->encodeBin(1, scmModel[offset]);
    }

    if (bCodeLast)
    {
        m_binIf->encodeBin(0, scmModel[offset]);
    }
}

void TEncSbac::xWriteEpExGolomb(UInt symbol, UInt count)
{
    UInt bins = 0;
    int numBins = 0;

    while (symbol >= (UInt)(1 << count))
    {
        bins = 2 * bins + 1;
        numBins++;
        symbol -= 1 << count;
        count++;
    }

    bins = 2 * bins + 0;
    numBins++;

    bins = (bins << count) | symbol;
    numBins += count;

    assert(numBins <= 32);
    m_binIf->encodeBinsEP(bins, numBins);
}

/** Coding of coeff_abs_level_minus3
 * \param symbol value of coeff_abs_level_minus3
 * \param ruiGoRiceParam reference to Rice parameter
 * \returns void
 */
void TEncSbac::xWriteCoefRemainExGolomb(UInt symbol, UInt &param)
{
    int codeNumber  = (int)symbol;
    UInt length;

    if (codeNumber < (COEF_REMAIN_BIN_REDUCTION << param))
    {
        length = codeNumber >> param;
        m_binIf->encodeBinsEP((1 << (length + 1)) - 2, length + 1);
        m_binIf->encodeBinsEP((codeNumber % (1 << param)), param);
    }
    else
    {
        length = param;
        codeNumber  = codeNumber - (COEF_REMAIN_BIN_REDUCTION << param);
        while (codeNumber >= (1 << length))
        {
            codeNumber -=  (1 << (length++));
        }

        m_binIf->encodeBinsEP((1 << (COEF_REMAIN_BIN_REDUCTION + length + 1 - param)) - 2, COEF_REMAIN_BIN_REDUCTION + length + 1 - param);
        m_binIf->encodeBinsEP(codeNumber, length);
    }
}

// SBAC RD
void  TEncSbac::load(TEncSbac* src)
{
    this->xCopyFrom(src);
}

void  TEncSbac::loadIntraDirModeLuma(TEncSbac* src)
{
    m_binIf->copyState(src->m_binIf);

    this->m_cuIntraPredSCModel.copyFrom(&src->m_cuIntraPredSCModel);
}

void  TEncSbac::store(TEncSbac* pDest)
{
    pDest->xCopyFrom(this);
}

void TEncSbac::xCopyFrom(TEncSbac* src)
{
    m_binIf->copyState(src->m_binIf);

    this->m_coeffCost = src->m_coeffCost;
    this->m_lastQp    = src->m_lastQp;

    memcpy(m_contextModels, src->m_contextModels, m_numContextModels * sizeof(ContextModel));
}

void TEncSbac::codeMVPIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList)
{
    int symbol = cu->getMVPIdx(eRefList, absPartIdx);
    int num = AMVP_MAX_NUM_CANDS;

    xWriteUnaryMaxSymbol(symbol, &m_mvpIdxSCModel.get(0), 1, num - 1);
}

void TEncSbac::codePartSize(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    PartSize partSize = cu->getPartitionSize(absPartIdx);

    if (cu->isIntra(absPartIdx))
    {
        if (depth == g_maxCUDepth - g_addCUDepth)
        {
            m_binIf->encodeBin(partSize == SIZE_2Nx2N ? 1 : 0, m_cuPartSizeSCModel.get(0));
        }
        return;
    }

    switch (partSize)
    {
    case SIZE_2Nx2N:
    {
        m_binIf->encodeBin(1, m_cuPartSizeSCModel.get(0));
        break;
    }
    case SIZE_2NxN:
    case SIZE_2NxnU:
    case SIZE_2NxnD:
    {
        m_binIf->encodeBin(0, m_cuPartSizeSCModel.get(0));
        m_binIf->encodeBin(1, m_cuPartSizeSCModel.get(1));
        if (cu->getSlice()->getSPS()->getAMPAcc(depth))
        {
            if (partSize == SIZE_2NxN)
            {
                m_binIf->encodeBin(1, m_cuAMPSCModel.get(0));
            }
            else
            {
                m_binIf->encodeBin(0, m_cuAMPSCModel.get(0));
                m_binIf->encodeBinEP((partSize == SIZE_2NxnU ? 0 : 1));
            }
        }
        break;
    }
    case SIZE_Nx2N:
    case SIZE_nLx2N:
    case SIZE_nRx2N:
    {
        m_binIf->encodeBin(0, m_cuPartSizeSCModel.get(0));
        m_binIf->encodeBin(0, m_cuPartSizeSCModel.get(1));
        if (depth == g_maxCUDepth - g_addCUDepth && !(cu->getWidth(absPartIdx) == 8 && cu->getHeight(absPartIdx) == 8))
        {
            m_binIf->encodeBin(1, m_cuPartSizeSCModel.get(2));
        }
        if (cu->getSlice()->getSPS()->getAMPAcc(depth))
        {
            if (partSize == SIZE_Nx2N)
            {
                m_binIf->encodeBin(1, m_cuAMPSCModel.get(0));
            }
            else
            {
                m_binIf->encodeBin(0, m_cuAMPSCModel.get(0));
                m_binIf->encodeBinEP((partSize == SIZE_nLx2N ? 0 : 1));
            }
        }
        break;
    }
    case SIZE_NxN:
    {
        if (depth == g_maxCUDepth - g_addCUDepth && !(cu->getWidth(absPartIdx) == 8 && cu->getHeight(absPartIdx) == 8))
        {
            m_binIf->encodeBin(0, m_cuPartSizeSCModel.get(0));
            m_binIf->encodeBin(0, m_cuPartSizeSCModel.get(1));
            m_binIf->encodeBin(0, m_cuPartSizeSCModel.get(2));
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
    int predMode = cu->getPredictionMode(absPartIdx);

    m_binIf->encodeBin(predMode == MODE_INTER ? 0 : 1, m_cuPredModeSCModel.get(0));
}

void TEncSbac::codeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx)
{
    UInt symbol = cu->getCUTransquantBypass(absPartIdx);

    m_binIf->encodeBin(symbol, m_cuTransquantBypassFlagSCModel.get(0));
}

/** code skip flag
 * \param cu
 * \param absPartIdx
 * \returns void
 */
void TEncSbac::codeSkipFlag(TComDataCU* cu, UInt absPartIdx)
{
    // get context function is here
    UInt symbol = cu->isSkipped(absPartIdx) ? 1 : 0;
    UInt ctxSkip = cu->getCtxSkipFlag(absPartIdx);

    m_binIf->encodeBin(symbol, m_cuSkipFlagSCModel.get(ctxSkip));
    DTRACE_CABAC_VL(g_nSymbolCounter++);
    DTRACE_CABAC_T("\tSkipFlag");
    DTRACE_CABAC_T("\tuiCtxSkip: ");
    DTRACE_CABAC_V(ctxSkip);
    DTRACE_CABAC_T("\tuiSymbol: ");
    DTRACE_CABAC_V(symbol);
    DTRACE_CABAC_T("\n");
}

/** code merge flag
 * \param cu
 * \param absPartIdx
 * \returns void
 */
void TEncSbac::codeMergeFlag(TComDataCU* cu, UInt absPartIdx)
{
    const UInt symbol = cu->getMergeFlag(absPartIdx) ? 1 : 0;

    m_binIf->encodeBin(symbol, m_cuMergeFlagExtSCModel.get(0));

    DTRACE_CABAC_VL(g_nSymbolCounter++);
    DTRACE_CABAC_T("\tMergeFlag: ");
    DTRACE_CABAC_V(symbol);
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
    UInt unaryIdx = cu->getMergeIndex(absPartIdx);
    UInt numCand = cu->getSlice()->getMaxNumMergeCand();

    if (numCand > 1)
    {
        for (UInt ui = 0; ui < numCand - 1; ++ui)
        {
            const UInt symbol = ui == unaryIdx ? 0 : 1;
            if (ui == 0)
            {
                m_binIf->encodeBin(symbol, m_cuMergeIdxExtSCModel.get(0));
            }
            else
            {
                m_binIf->encodeBinEP(symbol);
            }
            if (symbol == 0)
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

    UInt ctx           = cu->getCtxSplitFlag(absPartIdx, depth);
    UInt currSplitFlag = (cu->getDepth(absPartIdx) > depth) ? 1 : 0;

    assert(ctx < 3);
    m_binIf->encodeBin(currSplitFlag, m_cuSplitFlagSCModel.get(ctx));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tSplitFlag\n")
}

void TEncSbac::codeTransformSubdivFlag(UInt symbol, UInt ctx)
{
    m_binIf->encodeBin(symbol, m_cuTransSubdivFlagSCModel.get(ctx));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseTransformSubdivFlag()")
    DTRACE_CABAC_T("\tsymbol=")
    DTRACE_CABAC_V(symbol)
    DTRACE_CABAC_T("\tctx=")
    DTRACE_CABAC_V(ctx)
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

        m_binIf->encodeBin((predIdx[j] != -1) ? 1 : 0, m_cuIntraPredSCModel.get(0));
    }

    for (j = 0; j < partNum; j++)
    {
        if (predIdx[j] != -1)
        {
            m_binIf->encodeBinEP(predIdx[j] ? 1 : 0);
            if (predIdx[j])
            {
                m_binIf->encodeBinEP(predIdx[j] - 1);
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

            m_binIf->encodeBinsEP(dir[j], 5);
        }
    }
}

void TEncSbac::codeIntraDirChroma(TComDataCU* cu, UInt absPartIdx)
{
    UInt intraDirChroma = cu->getChromaIntraDir(absPartIdx);

    if (intraDirChroma == DM_CHROMA_IDX)
    {
        m_binIf->encodeBin(0, m_cuChromaPredSCModel.get(0));
    }
    else
    {
        UInt allowedChromaDir[NUM_CHROMA_MODE];
        cu->getAllowedChromaDir(absPartIdx, allowedChromaDir);

        for (int i = 0; i < NUM_CHROMA_MODE - 1; i++)
        {
            if (intraDirChroma == allowedChromaDir[i])
            {
                intraDirChroma = i;
                break;
            }
        }

        m_binIf->encodeBin(1, m_cuChromaPredSCModel.get(0));

        m_binIf->encodeBinsEP(intraDirChroma, 2);
    }
}

void TEncSbac::codeInterDir(TComDataCU* cu, UInt absPartIdx)
{
    const UInt interDir = cu->getInterDir(absPartIdx) - 1;
    const UInt ctx      = cu->getCtxInterDir(absPartIdx);

    if (cu->getPartitionSize(absPartIdx) == SIZE_2Nx2N || cu->getHeight(absPartIdx) != 8)
    {
        m_binIf->encodeBin(interDir == 2 ? 1 : 0, m_cuInterDirSCModel.get(ctx));
    }
    if (interDir < 2)
    {
        m_binIf->encodeBin(interDir, m_cuInterDirSCModel.get(4));
    }
}

void TEncSbac::codeRefFrmIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList)
{
    {
        int refFrame = cu->getCUMvField(eRefList)->getRefIdx(absPartIdx);
        int idx = 0;
        m_binIf->encodeBin((refFrame == 0 ? 0 : 1), m_cuRefPicSCModel.get(0));

        if (refFrame > 0)
        {
            UInt refNum = cu->getSlice()->getNumRefIdx(eRefList) - 2;
            idx++;
            refFrame--;
            for (UInt i = 0; i < refNum; ++i)
            {
                const UInt symbol = i == refFrame ? 0 : 1;
                if (i == 0)
                {
                    m_binIf->encodeBin(symbol, m_cuRefPicSCModel.get(idx));
                }
                else
                {
                    m_binIf->encodeBinEP(symbol);
                }
                if (symbol == 0)
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

    const TComCUMvField* cuMvField = cu->getCUMvField(eRefList);
    const int hor = cuMvField->getMvd(absPartIdx).x;
    const int ver = cuMvField->getMvd(absPartIdx).y;

    m_binIf->encodeBin(hor != 0 ? 1 : 0, m_cuMvdSCModel.get(0));
    m_binIf->encodeBin(ver != 0 ? 1 : 0, m_cuMvdSCModel.get(0));

    const bool bHorAbsGr0 = hor != 0;
    const bool bVerAbsGr0 = ver != 0;
    const UInt horAbs   = 0 > hor ? -hor : hor;
    const UInt verAbs   = 0 > ver ? -ver : ver;

    if (bHorAbsGr0)
    {
        m_binIf->encodeBin(horAbs > 1 ? 1 : 0, m_cuMvdSCModel.get(1));
    }

    if (bVerAbsGr0)
    {
        m_binIf->encodeBin(verAbs > 1 ? 1 : 0, m_cuMvdSCModel.get(1));
    }

    if (bHorAbsGr0)
    {
        if (horAbs > 1)
        {
            xWriteEpExGolomb(horAbs - 2, 1);
        }

        m_binIf->encodeBinEP(0 > hor ? 1 : 0);
    }

    if (bVerAbsGr0)
    {
        if (verAbs > 1)
        {
            xWriteEpExGolomb(verAbs - 2, 1);
        }

        m_binIf->encodeBinEP(0 > ver ? 1 : 0);
    }
}

void TEncSbac::codeDeltaQP(TComDataCU* cu, UInt absPartIdx)
{
    int dqp  = cu->getQP(absPartIdx) - cu->getRefQP(absPartIdx);

    int qpBdOffsetY =  cu->getSlice()->getSPS()->getQpBDOffsetY();

    dqp = (dqp + 78 + qpBdOffsetY + (qpBdOffsetY / 2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY / 2);

    UInt absDQp = (UInt)((dqp > 0) ? dqp  : (-dqp));
    UInt TUValue = X265_MIN((int)absDQp, CU_DQP_TU_CMAX);
    xWriteUnaryMaxSymbol(TUValue, &m_cuDeltaQpSCModel.get(0), 1, CU_DQP_TU_CMAX);
    if (absDQp >= CU_DQP_TU_CMAX)
    {
        xWriteEpExGolomb(absDQp - CU_DQP_TU_CMAX, CU_DQP_EG_k);
    }

    if (absDQp > 0)
    {
        UInt sign = (dqp > 0 ? 0 : 1);
        m_binIf->encodeBinEP(sign);
    }
}

void TEncSbac::codeQtCbf(TComDataCU* cu, UInt absPartIdx, TextType ttype, UInt trDepth)
{
    UInt cbf = cu->getCbf(absPartIdx, ttype, trDepth);
    UInt ctx = cu->getCtxQtCbf(ttype, trDepth);

    m_binIf->encodeBin(cbf, m_cuQtCbfSCModel.get((ttype ? TEXT_CHROMA : 0) * NUM_QT_CBF_CTX + ctx));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseQtCbf()")
    DTRACE_CABAC_T("\tsymbol=")
    DTRACE_CABAC_V(cbf)
    DTRACE_CABAC_T("\tctx=")
    DTRACE_CABAC_V(ctx)
    DTRACE_CABAC_T("\tetype=")
    DTRACE_CABAC_V(ttype)
    DTRACE_CABAC_T("\tuiAbsPartIdx=")
    DTRACE_CABAC_V(absPartIdx)
    DTRACE_CABAC_T("\n")
}

void TEncSbac::codeTransformSkipFlags(TComDataCU* cu, UInt absPartIdx, UInt width, UInt height, TextType ttype)
{
    if (cu->getCUTransquantBypass(absPartIdx))
    {
        return;
    }
    if (width != 4 || height != 4)
    {
        return;
    }

    UInt useTransformSkip = cu->getTransformSkip(absPartIdx, ttype);
    m_binIf->encodeBin(useTransformSkip, m_transformSkipSCModel.get((ttype ? TEXT_CHROMA : TEXT_LUMA) * NUM_TRANSFORMSKIP_FLAG_CTX));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseTransformSkip()");
    DTRACE_CABAC_T("\tsymbol=")
    DTRACE_CABAC_V(useTransformSkip)
    DTRACE_CABAC_T("\tAddr=")
    DTRACE_CABAC_V(cu->getAddr())
    DTRACE_CABAC_T("\tetype=")
    DTRACE_CABAC_V(ttype)
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
    UInt ipcm = (cu->getIPCMFlag(absPartIdx) == true) ? 1 : 0;

    bool writePCMSampleFlag = cu->getIPCMFlag(absPartIdx);

    m_binIf->encodeBinTrm(ipcm);

    if (writePCMSampleFlag)
    {
        m_binIf->encodePCMAlignBits();

        UInt minCoeffSize = cu->getPic()->getMinCUWidth() * cu->getPic()->getMinCUHeight();
        UInt lumaOffset   = minCoeffSize * absPartIdx;
        UInt chromaOffset = lumaOffset >> 2;
        Pel* pcmSample;
        UInt width;
        UInt height;
        UInt sampleBits;
        UInt x, y;

        pcmSample = cu->getPCMSampleY() + lumaOffset;
        width = cu->getWidth(absPartIdx);
        height = cu->getHeight(absPartIdx);
        sampleBits = cu->getSlice()->getSPS()->getPCMBitDepthLuma();

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                UInt uiSample = pcmSample[x];

                m_binIf->xWritePCMCode(uiSample, sampleBits);
            }

            pcmSample += width;
        }

        pcmSample = cu->getPCMSampleCb() + chromaOffset;
        width = cu->getWidth(absPartIdx) / 2;
        height = cu->getHeight(absPartIdx) / 2;
        sampleBits = cu->getSlice()->getSPS()->getPCMBitDepthChroma();

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                UInt sample = pcmSample[x];

                m_binIf->xWritePCMCode(sample, sampleBits);
            }

            pcmSample += width;
        }

        pcmSample = cu->getPCMSampleCr() + chromaOffset;
        width = cu->getWidth(absPartIdx) / 2;
        height = cu->getHeight(absPartIdx) / 2;
        sampleBits = cu->getSlice()->getSPS()->getPCMBitDepthChroma();

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                UInt sample = pcmSample[x];

                m_binIf->xWritePCMCode(sample, sampleBits);
            }

            pcmSample += width;
        }

        m_binIf->resetBac();
    }
}

void TEncSbac::codeQtRootCbf(TComDataCU* cu, UInt absPartIdx)
{
    UInt cbf = cu->getQtRootCbf(absPartIdx);
    UInt ctx = 0;

    m_binIf->encodeBin(cbf, m_cuQtRootCbfSCModel.get(ctx));
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseQtRootCbf()")
    DTRACE_CABAC_T("\tsymbol=")
    DTRACE_CABAC_V(cbf)
    DTRACE_CABAC_T("\tctx=")
    DTRACE_CABAC_V(ctx)
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

    m_binIf->encodeBin(cbf, m_cuQtCbfSCModel.get((ttype ? TEXT_CHROMA : 0) * NUM_QT_CBF_CTX + ctx));
}

void TEncSbac::codeQtRootCbfZero(TComDataCU*)
{
    // this function is only used to estimate the bits when cbf is 0
    // and will never be called when writing the bistream. do not need to write log
    UInt cbf = 0;
    UInt ctx = 0;

    m_binIf->encodeBin(cbf, m_cuQtRootCbfSCModel.get(ctx));
}

/** Encode (X,Y) position of the last significant coefficient
 * \param posx X component of last coefficient
 * \param posy Y component of last coefficient
 * \param width  Block width
 * \param height Block height
 * \param ttype plane type / luminance or chrominance
 * \param uiScanIdx scan type (zig-zag, hor, ver)
 * This method encodes the X and Y component within a block of the last significant coefficient.
 */
void TEncSbac::codeLastSignificantXY(UInt posx, UInt posy, int width, int height, TextType ttype, UInt scanIdx)
{
    assert((ttype == TEXT_LUMA) || (ttype == TEXT_CHROMA));
    // swap
    if (scanIdx == SCAN_VER)
    {
        std::swap(posx, posy);
    }

    UInt ctxLast;
    ContextModel *ctxX = &m_cuCtxLastX.get(ttype ? NUM_CTX_LAST_FLAG_XY : 0);
    ContextModel *ctxY = &m_cuCtxLastY.get(ttype ? NUM_CTX_LAST_FLAG_XY : 0);
    UInt groupIdxX    = g_groupIdx[posx];
    UInt groupIdxY    = g_groupIdx[posy];

    int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;
    blkSizeOffsetX = ttype ? 0 : (g_convertToBit[width] * 3 + ((g_convertToBit[width] + 1) >> 2));
    blkSizeOffsetY = ttype ? 0 : (g_convertToBit[height] * 3 + ((g_convertToBit[height] + 1) >> 2));
    shiftX = ttype ? g_convertToBit[width] : ((g_convertToBit[width] + 3) >> 2);
    shiftY = ttype ? g_convertToBit[height] : ((g_convertToBit[height] + 3) >> 2);
    // posX
    for (ctxLast = 0; ctxLast < groupIdxX; ctxLast++)
    {
        m_binIf->encodeBin(1, *(ctxX + blkSizeOffsetX + (ctxLast >> shiftX)));
    }

    if (groupIdxX < g_groupIdx[width - 1])
    {
        m_binIf->encodeBin(0, *(ctxX + blkSizeOffsetX + (ctxLast >> shiftX)));
    }

    // posY
    for (ctxLast = 0; ctxLast < groupIdxY; ctxLast++)
    {
        m_binIf->encodeBin(1, *(ctxY + blkSizeOffsetY + (ctxLast >> shiftY)));
    }

    if (groupIdxY < g_groupIdx[height - 1])
    {
        m_binIf->encodeBin(0, *(ctxY + blkSizeOffsetY + (ctxLast >> shiftY)));
    }
    if (groupIdxX > 3)
    {
        UInt count = (groupIdxX - 2) >> 1;
        posx       = posx - g_minInGroup[groupIdxX];
        m_binIf->encodeBinsEP(posx, count);
    }
    if (groupIdxY > 3)
    {
        UInt count = (groupIdxY - 2) >> 1;
        posy       = posy - g_minInGroup[groupIdxY];
        m_binIf->encodeBinsEP(posy, count);
    }
}

void TEncSbac::codeCoeffNxN(TComDataCU* cu, TCoeff* coeff, UInt absPartIdx, UInt width, UInt height, UInt depth, TextType ttype)
{
#if ENC_DEC_TRACE
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tparseCoeffNxN()\teType=")
    DTRACE_CABAC_V(ttype)
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

    if (width > m_slice->getSPS()->getMaxTrSize())
    {
        width  = m_slice->getSPS()->getMaxTrSize();
        height = m_slice->getSPS()->getMaxTrSize();
    }

    // compute number of significant coefficients
    UInt numSig = TEncEntropy::countNonZeroCoeffs(coeff, width * height);

    if (numSig == 0)
        return;
    if (cu->getSlice()->getPPS()->getUseTransformSkip())
    {
        codeTransformSkipFlags(cu, absPartIdx, width, height, ttype);
    }
    ttype = ttype == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA ;

    //----- encode significance map -----
    const UInt log2BlockSize = g_convertToBit[width] + 2;
    UInt scanIdx = cu->getCoefScanIdx(absPartIdx, width, ttype == TEXT_LUMA, cu->isIntra(absPartIdx));
    const UInt *scan = g_sigLastScan[scanIdx][log2BlockSize - 1];

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
        scanCG = g_sigLastScan[scanIdx][log2BlockSize > 3 ? log2BlockSize - 2 - 1 : 0];
        if (log2BlockSize == 3)
        {
            scanCG = g_sigLastScan8x8[scanIdx];
        }
        else if (log2BlockSize == 5)
        {
            scanCG = g_sigLastScanCG32x32;
        }
    }
    UInt sigCoeffGroupFlag[MLS_GRP_NUM];
    static const UInt shift = MLS_CG_SIZE >> 1;
    const UInt numBlkSide = width >> shift;

    ::memset(sigCoeffGroupFlag, 0, sizeof(UInt) * MLS_GRP_NUM);

    do
    {
        posLast = scan[++scanPosLast];

        // get L1 sig map
        UInt posy    = posLast >> log2BlockSize;
        UInt posx    = posLast - (posy << log2BlockSize);
        UInt blkIdx  = numBlkSide * (posy >> shift) + (posx >> shift);
        if (coeff[posLast])
        {
            sigCoeffGroupFlag[blkIdx] = 1;
        }

        numSig -= (coeff[posLast] != 0);
    }
    while (numSig > 0);

    // Code position of last coefficient
    int posLastY = posLast >> log2BlockSize;
    int posLastX = posLast - (posLastY << log2BlockSize);
    codeLastSignificantXY(posLastX, posLastY, width, height, ttype, scanIdx);

    //===== code significance flag =====
    ContextModel * const baseCoeffGroupCtx = &m_cuSigCoeffGroupSCModel.get(ttype ? NUM_SIG_CG_FLAG_CTX : 0);
    ContextModel * const baseCtx = (ttype == TEXT_LUMA) ? &m_cuSigSCModel.get(0) : &m_cuSigSCModel.get(NUM_SIG_FLAG_CTX_LUMA);

    const int lastScanSet      = scanPosLast >> LOG2_SCAN_SET_SIZE;
    UInt c1 = 1;
    UInt goRiceParam           = 0;
    int  scanPosSig            = scanPosLast;

    for (int subSet = lastScanSet; subSet >= 0; subSet--)
    {
        int numNonZero = 0;
        int subPos     = subSet << LOG2_SCAN_SET_SIZE;
        goRiceParam    = 0;
        int absCoeff[16];
        UInt coeffSigns = 0;

        int lastNZPosInCG = -1, firstNZPosInCG = SCAN_SET_SIZE;

        if (scanPosSig == scanPosLast)
        {
            absCoeff[0] = abs(coeff[posLast]);
            coeffSigns    = (coeff[posLast] < 0);
            numNonZero    = 1;
            lastNZPosInCG  = scanPosSig;
            firstNZPosInCG = scanPosSig;
            scanPosSig--;
        }

        // encode significant_coeffgroup_flag
        int cgBlkPos = scanCG[subSet];
        int cgPosY   = cgBlkPos / numBlkSide;
        int cgPosX   = cgBlkPos - (cgPosY * numBlkSide);
        if (subSet == lastScanSet || subSet == 0)
        {
            sigCoeffGroupFlag[cgBlkPos] = 1;
        }
        else
        {
            UInt sigCoeffGroup = (sigCoeffGroupFlag[cgBlkPos] != 0);
            UInt ctxSig = TComTrQuant::getSigCoeffGroupCtxInc(sigCoeffGroupFlag, cgPosX, cgPosY, width, height);
            m_binIf->encodeBin(sigCoeffGroup, baseCoeffGroupCtx[ctxSig]);
        }

        // encode significant_coeff_flag
        if (sigCoeffGroupFlag[cgBlkPos])
        {
            int patternSigCtx = TComTrQuant::calcPatternSigCtx(sigCoeffGroupFlag, cgPosX, cgPosY, width, height);
            UInt blkPos, posy, posx, sig, ctxSig;
            for (; scanPosSig >= subPos; scanPosSig--)
            {
                blkPos  = scan[scanPosSig];
                posy    = blkPos >> log2BlockSize;
                posx    = blkPos - (posy << log2BlockSize);
                sig     = (coeff[blkPos] != 0);
                if (scanPosSig > subPos || subSet == 0 || numNonZero)
                {
                    ctxSig  = TComTrQuant::getSigCtxInc(patternSigCtx, scanIdx, posx, posy, log2BlockSize, ttype);
                    m_binIf->encodeBin(sig, baseCtx[ctxSig]);
                }
                if (sig)
                {
                    absCoeff[numNonZero] = abs(coeff[blkPos]);
                    coeffSigns = 2 * coeffSigns + (coeff[blkPos] < 0);
                    numNonZero++;
                    if (lastNZPosInCG == -1)
                    {
                        lastNZPosInCG = scanPosSig;
                    }
                    firstNZPosInCG = scanPosSig;
                }
            }
        }
        else
        {
            scanPosSig = subPos - 1;
        }

        if (numNonZero > 0)
        {
            bool signHidden = (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD);
            UInt ctxSet = (subSet > 0 && ttype == TEXT_LUMA) ? 2 : 0;

            if (c1 == 0)
            {
                ctxSet++;
            }
            c1 = 1;
            ContextModel *baseCtxMod = (ttype == TEXT_LUMA) ? &m_cuOneSCModel.get(4 * ctxSet) : &m_cuOneSCModel.get(NUM_ONE_FLAG_CTX_LUMA + 4 * ctxSet);

            int numC1Flag = X265_MIN(numNonZero, C1FLAG_NUMBER);
            int firstC2FlagIdx = -1;
            for (int idx = 0; idx < numC1Flag; idx++)
            {
                UInt symbol = absCoeff[idx] > 1;
                m_binIf->encodeBin(symbol, baseCtxMod[c1]);
                if (symbol)
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
                baseCtxMod = (ttype == TEXT_LUMA) ? &m_cuAbsSCModel.get(ctxSet) : &m_cuAbsSCModel.get(NUM_ABS_FLAG_CTX_LUMA + ctxSet);
                if (firstC2FlagIdx != -1)
                {
                    UInt symbol = absCoeff[firstC2FlagIdx] > 2;
                    m_binIf->encodeBin(symbol, baseCtxMod[0]);
                }
            }

            if (beValid && signHidden)
            {
                m_binIf->encodeBinsEP((coeffSigns >> 1), numNonZero - 1);
            }
            else
            {
                m_binIf->encodeBinsEP(coeffSigns, numNonZero);
            }

            int firstCoeff2 = 1;
            if (c1 == 0 || numNonZero > C1FLAG_NUMBER)
            {
                for (int idx = 0; idx < numNonZero; idx++)
                {
                    UInt baseLevel  = (idx < C1FLAG_NUMBER) ? (2 + firstCoeff2) : 1;

                    if (absCoeff[idx] >= baseLevel)
                    {
                        xWriteCoefRemainExGolomb(absCoeff[idx] - baseLevel, goRiceParam);
                        if (absCoeff[idx] > 3 * (1 << goRiceParam))
                        {
                            goRiceParam = std::min<UInt>(goRiceParam + 1, 4);
                        }
                    }
                    if (absCoeff[idx] >= 2)
                    {
                        firstCoeff2 = 0;
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
    m_binIf->encodeBinEP(code);
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
        m_binIf->encodeBinEP(0);
    }
    else
    {
        m_binIf->encodeBinEP(1);
        for (i = 0; i < code - 1; i++)
        {
            m_binIf->encodeBinEP(1);
        }

        if (bCodeLast)
        {
            m_binIf->encodeBinEP(0);
        }
    }
}

/** Code SAO EO class or BO band position
 * \param length
 * \param code
 */
void TEncSbac::codeSaoUflc(UInt length, UInt code)
{
    m_binIf->encodeBinsEP(code, length);
}

/** Code SAO merge flags
 * \param code
 * \param uiCompIdx
 */
void TEncSbac::codeSaoMerge(UInt code)
{
    if (code == 0)
    {
        m_binIf->encodeBin(0,  m_saoMergeSCModel.get(0));
    }
    else
    {
        m_binIf->encodeBin(1,  m_saoMergeSCModel.get(0));
    }
}

/** Code SAO type index
 * \param code
 */
void TEncSbac::codeSaoTypeIdx(UInt code)
{
    if (code == 0)
    {
        m_binIf->encodeBin(0, m_saoTypeIdxSCModel.get(0));
    }
    else
    {
        m_binIf->encodeBin(1, m_saoTypeIdxSCModel.get(0));
        m_binIf->encodeBinEP(code <= 4 ? 1 : 0);
    }
}

/*!
 ****************************************************************************
 * \brief
 *   estimate bit cost for CBP, significant map and significant coefficients
 ****************************************************************************
 */
void TEncSbac::estBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype)
{
    estCBFBit(estBitsSbac);

    estSignificantCoeffGroupMapBit(estBitsSbac, ttype);

    // encode significance map
    estSignificantMapBit(estBitsSbac, width, height, ttype);

    // encode significant coefficients
    estSignificantCoefficientsBit(estBitsSbac, ttype);
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost for each CBP bit
 ****************************************************************************
 */
void TEncSbac::estCBFBit(estBitsSbacStruct* estBitsSbac)
{
    ContextModel *ctx = &m_cuQtCbfSCModel.get(0);

    for (UInt uiCtxInc = 0; uiCtxInc < 3 * NUM_QT_CBF_CTX; uiCtxInc++)
    {
        estBitsSbac->blockCbpBits[uiCtxInc][0] = ctx[uiCtxInc].getEntropyBits(0);
        estBitsSbac->blockCbpBits[uiCtxInc][1] = ctx[uiCtxInc].getEntropyBits(1);
    }

    ctx = &m_cuQtRootCbfSCModel.get(0);

    for (UInt uiCtxInc = 0; uiCtxInc < 4; uiCtxInc++)
    {
        estBitsSbac->blockRootCbpBits[uiCtxInc][0] = ctx[uiCtxInc].getEntropyBits(0);
        estBitsSbac->blockRootCbpBits[uiCtxInc][1] = ctx[uiCtxInc].getEntropyBits(1);
    }
}

/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient group map
 ****************************************************************************
 */
void TEncSbac::estSignificantCoeffGroupMapBit(estBitsSbacStruct* estBitsSbac, TextType ttype)
{
    assert((ttype == TEXT_LUMA) || (ttype == TEXT_CHROMA));
    int firstCtx = 0, numCtx = NUM_SIG_CG_FLAG_CTX;

    for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
    {
        for (UInt bin = 0; bin < 2; bin++)
        {
            estBitsSbac->significantCoeffGroupBits[ctxIdx][bin] = m_cuSigCoeffGroupSCModel.get((ttype ? NUM_SIG_CG_FLAG_CTX : 0) + ctxIdx).getEntropyBits(bin);
        }
    }
}

/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient map
 ****************************************************************************
 */
void TEncSbac::estSignificantMapBit(estBitsSbacStruct* estBitsSbac, int width, int height, TextType ttype)
{
    int firstCtx = 1, numCtx = 8;

    if (X265_MAX(width, height) >= 16)
    {
        firstCtx = (ttype == TEXT_LUMA) ? 21 : 12;
        numCtx = (ttype == TEXT_LUMA) ? 6 : 3;
    }
    else if (width == 8)
    {
        firstCtx = 9;
        numCtx = (ttype == TEXT_LUMA) ? 12 : 3;
    }

    if (ttype == TEXT_LUMA)
    {
        for (UInt bin = 0; bin < 2; bin++)
        {
            estBitsSbac->significantBits[0][bin] = m_cuSigSCModel.get(0).getEntropyBits(bin);
        }

        for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
        {
            for (UInt bin = 0; bin < 2; bin++)
            {
                estBitsSbac->significantBits[ctxIdx][bin] = m_cuSigSCModel.get(ctxIdx).getEntropyBits(bin);
            }
        }
    }
    else
    {
        for (UInt bin = 0; bin < 2; bin++)
        {
            estBitsSbac->significantBits[0][bin] = m_cuSigSCModel.get(NUM_SIG_FLAG_CTX_LUMA + 0).getEntropyBits(bin);
        }

        for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
        {
            for (UInt bin = 0; bin < 2; bin++)
            {
                estBitsSbac->significantBits[ctxIdx][bin] = m_cuSigSCModel.get(NUM_SIG_FLAG_CTX_LUMA + ctxIdx).getEntropyBits(bin);
            }
        }
    }
    int bitsX = 0, bitsY = 0;
    int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;

    blkSizeOffsetX = ttype ? 0 : (g_convertToBit[width] * 3 + ((g_convertToBit[width] + 1) >> 2));
    blkSizeOffsetY = ttype ? 0 : (g_convertToBit[height] * 3 + ((g_convertToBit[height] + 1) >> 2));
    shiftX = ttype ? g_convertToBit[width] : ((g_convertToBit[width] + 3) >> 2);
    shiftY = ttype ? g_convertToBit[height] : ((g_convertToBit[height] + 3) >> 2);

    assert((ttype == TEXT_LUMA) || (ttype == TEXT_CHROMA));
    int ctx;
    for (ctx = 0; ctx < g_groupIdx[width - 1]; ctx++)
    {
        int ctxOffset = blkSizeOffsetX + (ctx >> shiftX);
        estBitsSbac->lastXBits[ctx] = bitsX + m_cuCtxLastX.get((ttype ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset).getEntropyBits(0);
        bitsX += m_cuCtxLastX.get((ttype ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset).getEntropyBits(1);
    }

    estBitsSbac->lastXBits[ctx] = bitsX;
    for (ctx = 0; ctx < g_groupIdx[height - 1]; ctx++)
    {
        int ctxOffset = blkSizeOffsetY + (ctx >> shiftY);
        estBitsSbac->lastYBits[ctx] = bitsY + m_cuCtxLastY.get((ttype ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset).getEntropyBits(0);
        bitsY += m_cuCtxLastY.get((ttype ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset).getEntropyBits(1);
    }

    estBitsSbac->lastYBits[ctx] = bitsY;
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost of significant coefficient
 ****************************************************************************
 */
void TEncSbac::estSignificantCoefficientsBit(estBitsSbacStruct* estBitsSbac, TextType ttype)
{
    if (ttype == TEXT_LUMA)
    {
        ContextModel *ctxOne = &m_cuOneSCModel.get(0);
        ContextModel *ctxAbs = &m_cuAbsSCModel.get(0);

        for (int ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_LUMA; ctxIdx++)
        {
            estBitsSbac->greaterOneBits[ctxIdx][0] = ctxOne[ctxIdx].getEntropyBits(0);
            estBitsSbac->greaterOneBits[ctxIdx][1] = ctxOne[ctxIdx].getEntropyBits(1);
        }

        for (int ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_LUMA; ctxIdx++)
        {
            estBitsSbac->levelAbsBits[ctxIdx][0] = ctxAbs[ctxIdx].getEntropyBits(0);
            estBitsSbac->levelAbsBits[ctxIdx][1] = ctxAbs[ctxIdx].getEntropyBits(1);
        }
    }
    else
    {
        ContextModel *ctxOne = &m_cuOneSCModel.get(NUM_ONE_FLAG_CTX_LUMA);
        ContextModel *ctxAbs = &m_cuAbsSCModel.get(NUM_ABS_FLAG_CTX_LUMA);

        for (int ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_CHROMA; ctxIdx++)
        {
            estBitsSbac->greaterOneBits[ctxIdx][0] = ctxOne[ctxIdx].getEntropyBits(0);
            estBitsSbac->greaterOneBits[ctxIdx][1] = ctxOne[ctxIdx].getEntropyBits(1);
        }

        for (int ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_CHROMA; ctxIdx++)
        {
            estBitsSbac->levelAbsBits[ctxIdx][0] = ctxAbs[ctxIdx].getEntropyBits(0);
            estBitsSbac->levelAbsBits[ctxIdx][1] = ctxAbs[ctxIdx].getEntropyBits(1);
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

void  TEncSbac::loadContexts(TEncSbac* src)
{
    this->xCopyContextsFrom(src);
}

//! \}
