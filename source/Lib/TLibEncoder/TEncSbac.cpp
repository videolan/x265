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

static void initBuffer(ContextModel* contextModel, SliceType sliceType, int qp, UChar* ctxModel, int size)
{
    ctxModel += sliceType * size;

    for (int n = 0; n < size; n++)
    {
        contextModel[n].init(qp, ctxModel[n]);
        contextModel[n].setBinsCoded(0);
    }
}

static UInt calcCost(ContextModel *contextModel, SliceType sliceType, int qp, UChar* ctxModel, int size)
{
    UInt cost = 0;

    ctxModel += sliceType * size;

    for (int n = 0; n < size; n++)
    {
        ContextModel tmpContextModel;
        tmpContextModel.init(qp, ctxModel[n]);

        // Map the 64 CABAC states to their corresponding probability values
        static double aStateToProbLPS[] = { 0.50000000, 0.47460857, 0.45050660, 0.42762859, 0.40591239, 0.38529900, 0.36573242, 0.34715948, 0.32952974, 0.31279528, 0.29691064, 0.28183267, 0.26752040, 0.25393496, 0.24103941, 0.22879875, 0.21717969, 0.20615069, 0.19568177, 0.18574449, 0.17631186, 0.16735824, 0.15885931, 0.15079198, 0.14313433, 0.13586556, 0.12896592, 0.12241667, 0.11620000, 0.11029903, 0.10469773, 0.09938088, 0.09433404, 0.08954349, 0.08499621, 0.08067986, 0.07658271, 0.07269362, 0.06900203, 0.06549791, 0.06217174, 0.05901448, 0.05601756, 0.05317283, 0.05047256, 0.04790942, 0.04547644, 0.04316702, 0.04097487, 0.03889405, 0.03691890, 0.03504406, 0.03326442, 0.03157516, 0.02997168, 0.02844963, 0.02700488, 0.02563349, 0.02433175, 0.02309612, 0.02192323, 0.02080991, 0.01975312, 0.01875000 };

        double probLPS          = aStateToProbLPS[contextModel[n].getState()];
        double prob0, prob1;
        if (contextModel[n].getMps() == 1)
        {
            prob0 = probLPS;
            prob1 = 1.0 - prob0;
        }
        else
        {
            prob1 = probLPS;
            prob0 = 1.0 - prob1;
        }

        if (contextModel[n].getBinsCoded() > 0)
        {
            cost += (UInt)(prob0 * tmpContextModel.getEntropyBits(0) + prob1 * tmpContextModel.getEntropyBits(1));
        }
    }

    return cost;
}

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSbac::TEncSbac()
// new structure here
    : m_bitIf(NULL)
    , m_slice(NULL)
    , m_binIf(NULL)
    , m_coeffCost(0)
{
    assert(MAX_OFF_CTX_MOD <= MAX_NUM_CTX_MOD);
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

    initBuffer(&m_contextModels[OFF_SPLIT_FLAG_CTX], sliceType, qp, (UChar*)INIT_SPLIT_FLAG, NUM_SPLIT_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_SKIP_FLAG_CTX], sliceType, qp, (UChar*)INIT_SKIP_FLAG, NUM_SKIP_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_MERGE_FLAG_EXT_CTX], sliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT, NUM_MERGE_FLAG_EXT_CTX);
    initBuffer(&m_contextModels[OFF_MERGE_IDX_EXT_CTX], sliceType, qp, (UChar*)INIT_MERGE_IDX_EXT, NUM_MERGE_IDX_EXT_CTX);
    initBuffer(&m_contextModels[OFF_PART_SIZE_CTX], sliceType, qp, (UChar*)INIT_PART_SIZE, NUM_PART_SIZE_CTX);
    initBuffer(&m_contextModels[OFF_PRED_MODE_CTX], sliceType, qp, (UChar*)INIT_PRED_MODE, NUM_PRED_MODE_CTX);
    initBuffer(&m_contextModels[OFF_ADI_CTX], sliceType, qp, (UChar*)INIT_INTRA_PRED_MODE, NUM_ADI_CTX);
    initBuffer(&m_contextModels[OFF_CHROMA_PRED_CTX], sliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE, NUM_CHROMA_PRED_CTX);
    initBuffer(&m_contextModels[OFF_DELTA_QP_CTX], sliceType, qp, (UChar*)INIT_DQP, NUM_DELTA_QP_CTX);
    initBuffer(&m_contextModels[OFF_INTER_DIR_CTX], sliceType, qp, (UChar*)INIT_INTER_DIR, NUM_INTER_DIR_CTX);
    initBuffer(&m_contextModels[OFF_REF_NO_CTX], sliceType, qp, (UChar*)INIT_REF_PIC, NUM_REF_NO_CTX);
    initBuffer(&m_contextModels[OFF_MV_RES_CTX], sliceType, qp, (UChar*)INIT_MVD, NUM_MV_RES_CTX);
    initBuffer(&m_contextModels[OFF_QT_CBF_CTX], sliceType, qp, (UChar*)INIT_QT_CBF, 2 * NUM_QT_CBF_CTX);
    initBuffer(&m_contextModels[OFF_TRANS_SUBDIV_FLAG_CTX], sliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG, NUM_TRANS_SUBDIV_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_QT_ROOT_CBF_CTX], sliceType, qp, (UChar*)INIT_QT_ROOT_CBF, NUM_QT_ROOT_CBF_CTX);
    initBuffer(&m_contextModels[OFF_SIG_CG_FLAG_CTX], sliceType, qp, (UChar*)INIT_SIG_CG_FLAG, 2 * NUM_SIG_CG_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_SIG_FLAG_CTX], sliceType, qp, (UChar*)INIT_SIG_FLAG, NUM_SIG_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_CTX_LAST_FLAG_X], sliceType, qp, (UChar*)INIT_LAST, 2 * NUM_CTX_LAST_FLAG_XY);
    initBuffer(&m_contextModels[OFF_CTX_LAST_FLAG_Y], sliceType, qp, (UChar*)INIT_LAST, 2 * NUM_CTX_LAST_FLAG_XY);
    initBuffer(&m_contextModels[OFF_ONE_FLAG_CTX], sliceType, qp, (UChar*)INIT_ONE_FLAG, NUM_ONE_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_ABS_FLAG_CTX], sliceType, qp, (UChar*)INIT_ABS_FLAG, NUM_ABS_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_MVP_IDX_CTX], sliceType, qp, (UChar*)INIT_MVP_IDX, NUM_MVP_IDX_CTX);
    initBuffer(&m_contextModels[OFF_CU_AMP_CTX], sliceType, qp, (UChar*)INIT_CU_AMP_POS, NUM_CU_AMP_CTX);
    initBuffer(&m_contextModels[OFF_SAO_MERGE_FLAG_CTX], sliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG, NUM_SAO_MERGE_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_SAO_TYPE_IDX_CTX], sliceType, qp, (UChar*)INIT_SAO_TYPE_IDX, NUM_SAO_TYPE_IDX_CTX);
    initBuffer(&m_contextModels[OFF_TRANSFORMSKIP_FLAG_CTX], sliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG, 2 * NUM_TRANSFORMSKIP_FLAG_CTX);
    initBuffer(&m_contextModels[OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX], sliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG, NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX);
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

            curCost  = calcCost(&m_contextModels[OFF_SPLIT_FLAG_CTX], curSliceType, qp, (UChar*)INIT_SPLIT_FLAG, NUM_SPLIT_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_SKIP_FLAG_CTX], curSliceType, qp, (UChar*)INIT_SKIP_FLAG, NUM_SKIP_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_MERGE_FLAG_EXT_CTX], curSliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT, NUM_MERGE_FLAG_EXT_CTX);
            curCost += calcCost(&m_contextModels[OFF_MERGE_IDX_EXT_CTX], curSliceType, qp, (UChar*)INIT_MERGE_IDX_EXT, NUM_MERGE_IDX_EXT_CTX);
            curCost += calcCost(&m_contextModels[OFF_PART_SIZE_CTX], curSliceType, qp, (UChar*)INIT_PART_SIZE, NUM_PART_SIZE_CTX);
            curCost += calcCost(&m_contextModels[OFF_PRED_MODE_CTX], curSliceType, qp, (UChar*)INIT_PRED_MODE, NUM_PRED_MODE_CTX);
            curCost += calcCost(&m_contextModels[OFF_ADI_CTX], curSliceType, qp, (UChar*)INIT_INTRA_PRED_MODE, NUM_ADI_CTX);
            curCost += calcCost(&m_contextModels[OFF_CHROMA_PRED_CTX], curSliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE, NUM_CHROMA_PRED_CTX);
            curCost += calcCost(&m_contextModels[OFF_DELTA_QP_CTX], curSliceType, qp, (UChar*)INIT_DQP, NUM_DELTA_QP_CTX);
            curCost += calcCost(&m_contextModels[OFF_INTER_DIR_CTX], curSliceType, qp, (UChar*)INIT_INTER_DIR, NUM_INTER_DIR_CTX);
            curCost += calcCost(&m_contextModels[OFF_REF_NO_CTX], curSliceType, qp, (UChar*)INIT_REF_PIC, NUM_REF_NO_CTX);
            curCost += calcCost(&m_contextModels[OFF_MV_RES_CTX], curSliceType, qp, (UChar*)INIT_MVD, NUM_MV_RES_CTX);
            curCost += calcCost(&m_contextModels[OFF_QT_CBF_CTX], curSliceType, qp, (UChar*)INIT_QT_CBF, 2 * NUM_QT_CBF_CTX);
            curCost += calcCost(&m_contextModels[OFF_TRANS_SUBDIV_FLAG_CTX], curSliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG, NUM_TRANS_SUBDIV_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_QT_ROOT_CBF_CTX], curSliceType, qp, (UChar*)INIT_QT_ROOT_CBF, NUM_QT_ROOT_CBF_CTX);
            curCost += calcCost(&m_contextModels[OFF_SIG_CG_FLAG_CTX], curSliceType, qp, (UChar*)INIT_SIG_CG_FLAG, 2 * NUM_SIG_CG_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_SIG_FLAG_CTX], curSliceType, qp, (UChar*)INIT_SIG_FLAG, NUM_SIG_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_CTX_LAST_FLAG_X], curSliceType, qp, (UChar*)INIT_LAST, 2 * NUM_CTX_LAST_FLAG_XY);
            curCost += calcCost(&m_contextModels[OFF_CTX_LAST_FLAG_Y], curSliceType, qp, (UChar*)INIT_LAST, 2 * NUM_CTX_LAST_FLAG_XY);
            curCost += calcCost(&m_contextModels[OFF_ONE_FLAG_CTX], curSliceType, qp, (UChar*)INIT_ONE_FLAG, NUM_ONE_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_ABS_FLAG_CTX], curSliceType, qp, (UChar*)INIT_ABS_FLAG, NUM_ABS_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_MVP_IDX_CTX], curSliceType, qp, (UChar*)INIT_MVP_IDX, NUM_MVP_IDX_CTX);
            curCost += calcCost(&m_contextModels[OFF_CU_AMP_CTX], curSliceType, qp, (UChar*)INIT_CU_AMP_POS, NUM_CU_AMP_CTX);
            curCost += calcCost(&m_contextModels[OFF_SAO_MERGE_FLAG_CTX], curSliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG, NUM_SAO_MERGE_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_SAO_TYPE_IDX_CTX], curSliceType, qp, (UChar*)INIT_SAO_TYPE_IDX, NUM_SAO_TYPE_IDX_CTX);
            curCost += calcCost(&m_contextModels[OFF_TRANSFORMSKIP_FLAG_CTX], curSliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG, 2 * NUM_TRANSFORMSKIP_FLAG_CTX);
            curCost += calcCost(&m_contextModels[OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX], curSliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG, NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX);
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

    ::memcpy(&this->m_contextModels[OFF_ADI_CTX], &src->m_contextModels[OFF_ADI_CTX], sizeof(ContextModel) * NUM_ADI_CTX);
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

    memcpy(m_contextModels, src->m_contextModels, MAX_OFF_CTX_MOD * sizeof(ContextModel));
}

void TEncSbac::codeMVPIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList)
{
    int symbol = cu->getMVPIdx(eRefList, absPartIdx);
    int num = AMVP_MAX_NUM_CANDS;

    xWriteUnaryMaxSymbol(symbol, &m_contextModels[OFF_MVP_IDX_CTX], 1, num - 1);
}

void TEncSbac::codePartSize(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    PartSize partSize = cu->getPartitionSize(absPartIdx);

    if (cu->isIntra(absPartIdx))
    {
        if (depth == g_maxCUDepth - g_addCUDepth)
        {
            m_binIf->encodeBin(partSize == SIZE_2Nx2N ? 1 : 0, m_contextModels[OFF_PART_SIZE_CTX]);
        }
        return;
    }

    switch (partSize)
    {
    case SIZE_2Nx2N:
    {
        m_binIf->encodeBin(1, m_contextModels[OFF_PART_SIZE_CTX]);
        break;
    }
    case SIZE_2NxN:
    case SIZE_2NxnU:
    case SIZE_2NxnD:
    {
        m_binIf->encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 0]);
        m_binIf->encodeBin(1, m_contextModels[OFF_PART_SIZE_CTX + 1]);
        if (cu->getSlice()->getSPS()->getAMPAcc(depth))
        {
            m_binIf->encodeBin((partSize == SIZE_2NxN) ? 1 : 0, m_contextModels[OFF_CU_AMP_CTX]);
            if (partSize != SIZE_2NxN)
            {
                m_binIf->encodeBinEP((partSize == SIZE_2NxnU ? 0 : 1));
            }
        }
        break;
    }
    case SIZE_Nx2N:
    case SIZE_nLx2N:
    case SIZE_nRx2N:
    {
        m_binIf->encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 0]);
        m_binIf->encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 1]);
        if (depth == g_maxCUDepth - g_addCUDepth && !(cu->getWidth(absPartIdx) == 8 && cu->getHeight(absPartIdx) == 8))
        {
            m_binIf->encodeBin(1, m_contextModels[OFF_PART_SIZE_CTX + 2]);
        }
        if (cu->getSlice()->getSPS()->getAMPAcc(depth))
        {
            m_binIf->encodeBin((partSize == SIZE_Nx2N) ? 1 : 0, m_contextModels[OFF_CU_AMP_CTX]);
            if (partSize != SIZE_Nx2N)
            {
                m_binIf->encodeBinEP((partSize == SIZE_nLx2N ? 0 : 1));
            }
        }
        break;
    }
    case SIZE_NxN:
    {
        if (depth == g_maxCUDepth - g_addCUDepth && !(cu->getWidth(absPartIdx) == 8 && cu->getHeight(absPartIdx) == 8))
        {
            m_binIf->encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 0]);
            m_binIf->encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 1]);
            m_binIf->encodeBin(0, m_contextModels[OFF_PART_SIZE_CTX + 2]);
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

    m_binIf->encodeBin(predMode == MODE_INTER ? 0 : 1, m_contextModels[OFF_PRED_MODE_CTX]);
}

void TEncSbac::codeCUTransquantBypassFlag(TComDataCU* cu, UInt absPartIdx)
{
    UInt symbol = cu->getCUTransquantBypass(absPartIdx);

    m_binIf->encodeBin(symbol, m_contextModels[OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX]);
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

    m_binIf->encodeBin(symbol, m_contextModels[OFF_SKIP_FLAG_CTX + ctxSkip]);
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

    m_binIf->encodeBin(symbol, m_contextModels[OFF_MERGE_FLAG_EXT_CTX]);

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
        m_binIf->encodeBin((unaryIdx != 0), m_contextModels[OFF_MERGE_IDX_EXT_CTX]);

        assert(unaryIdx < numCand);

        if (unaryIdx != 0)
        {
            UInt mask = (1 << unaryIdx) - 2;
            mask >>= (unaryIdx == numCand - 1) ? 1 : 0;
            m_binIf->encodeBinsEP(mask, unaryIdx - (unaryIdx == numCand - 1));
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
    m_binIf->encodeBin(currSplitFlag, m_contextModels[OFF_SPLIT_FLAG_CTX + ctx]);
    DTRACE_CABAC_VL(g_nSymbolCounter++)
    DTRACE_CABAC_T("\tSplitFlag\n")
}

void TEncSbac::codeTransformSubdivFlag(UInt symbol, UInt ctx)
{
    m_binIf->encodeBin(symbol, m_contextModels[OFF_TRANS_SUBDIV_FLAG_CTX + ctx]);
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
    int predIdx[4] = { -1, -1, -1, -1 };
    PartSize mode = cu->getPartitionSize(absPartIdx);
    UInt partNum = isMultiple ? (mode == SIZE_NxN ? 4 : 1) : 1;
    UInt partOffset = (cu->getPic()->getNumPartInCU() >> (cu->getDepth(absPartIdx) << 1)) >> 2;

    for (j = 0; j < partNum; j++)
    {
        dir[j] = cu->getLumaIntraDir(absPartIdx + partOffset * j);
        cu->getIntraDirLumaPredictor(absPartIdx + partOffset * j, preds[j]);
        for (UInt i = 0; i < 3; i++)
        {
            if (dir[j] == preds[j][i])
            {
                predIdx[j] = i;
            }
        }

        m_binIf->encodeBin((predIdx[j] != -1) ? 1 : 0, m_contextModels[OFF_ADI_CTX]);
    }

    for (j = 0; j < partNum; j++)
    {
        if (predIdx[j] != -1)
        {
            assert((predIdx[j] >= 0) && (predIdx[j] <= 2));
            // NOTE: Mapping
            //       0 = 0
            //       1 = 10
            //       2 = 11
            int nonzero = (!!predIdx[j]);
            m_binIf->encodeBinsEP(predIdx[j] + nonzero, 1 + nonzero);
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
            dir[j] += (dir[j] > preds[j][2]) ? -1 : 0;
            dir[j] += (dir[j] > preds[j][1]) ? -1 : 0;
            dir[j] += (dir[j] > preds[j][0]) ? -1 : 0;

            m_binIf->encodeBinsEP(dir[j], 5);
        }
    }
}

void TEncSbac::codeIntraDirChroma(TComDataCU* cu, UInt absPartIdx)
{
    UInt intraDirChroma = cu->getChromaIntraDir(absPartIdx);

    if (intraDirChroma == DM_CHROMA_IDX)
    {
        m_binIf->encodeBin(0, m_contextModels[OFF_CHROMA_PRED_CTX]);
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

        m_binIf->encodeBin(1, m_contextModels[OFF_CHROMA_PRED_CTX]);

        m_binIf->encodeBinsEP(intraDirChroma, 2);
    }
}

void TEncSbac::codeInterDir(TComDataCU* cu, UInt absPartIdx)
{
    const UInt interDir = cu->getInterDir(absPartIdx) - 1;
    const UInt ctx      = cu->getCtxInterDir(absPartIdx);

    if (cu->getPartitionSize(absPartIdx) == SIZE_2Nx2N || cu->getHeight(absPartIdx) != 8)
    {
        m_binIf->encodeBin(interDir == 2 ? 1 : 0, m_contextModels[OFF_INTER_DIR_CTX + ctx]);
    }
    if (interDir < 2)
    {
        m_binIf->encodeBin(interDir, m_contextModels[OFF_INTER_DIR_CTX + 4]);
    }
}

void TEncSbac::codeRefFrmIdx(TComDataCU* cu, UInt absPartIdx, RefPicList eRefList)
{
    {
        int refFrame = cu->getCUMvField(eRefList)->getRefIdx(absPartIdx);
        int idx = 0;
        m_binIf->encodeBin((refFrame == 0 ? 0 : 1), m_contextModels[OFF_REF_NO_CTX]);

        if (refFrame > 0)
        {
            UInt refNum = cu->getSlice()->getNumRefIdx(eRefList) - 2;
            idx++;
            refFrame--;
            // TODO: reference codeMergeIndex() to improvement
            for (UInt i = 0; i < refNum; ++i)
            {
                const UInt symbol = i == refFrame ? 0 : 1;
                if (i == 0)
                {
                    m_binIf->encodeBin(symbol, m_contextModels[OFF_REF_NO_CTX + idx]);
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

    m_binIf->encodeBin(hor != 0 ? 1 : 0, m_contextModels[OFF_MV_RES_CTX]);
    m_binIf->encodeBin(ver != 0 ? 1 : 0, m_contextModels[OFF_MV_RES_CTX]);

    const bool bHorAbsGr0 = hor != 0;
    const bool bVerAbsGr0 = ver != 0;
    const UInt horAbs   = 0 > hor ? -hor : hor;
    const UInt verAbs   = 0 > ver ? -ver : ver;

    if (bHorAbsGr0)
    {
        m_binIf->encodeBin(horAbs > 1 ? 1 : 0, m_contextModels[OFF_MV_RES_CTX + 1]);
    }

    if (bVerAbsGr0)
    {
        m_binIf->encodeBin(verAbs > 1 ? 1 : 0, m_contextModels[OFF_MV_RES_CTX + 1]);
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
    xWriteUnaryMaxSymbol(TUValue, &m_contextModels[OFF_DELTA_QP_CTX], 1, CU_DQP_TU_CMAX);
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

    m_binIf->encodeBin(cbf, m_contextModels[OFF_QT_CBF_CTX + (ttype ? TEXT_CHROMA : 0) * NUM_QT_CBF_CTX + ctx]);
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
    m_binIf->encodeBin(useTransformSkip, m_contextModels[OFF_TRANSFORMSKIP_FLAG_CTX + (ttype ? TEXT_CHROMA : TEXT_LUMA) * NUM_TRANSFORMSKIP_FLAG_CTX]);
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

    m_binIf->encodeBin(cbf, m_contextModels[OFF_QT_ROOT_CBF_CTX + ctx]);
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

    m_binIf->encodeBin(cbf, m_contextModels[OFF_QT_CBF_CTX + (ttype ? TEXT_CHROMA : 0) * NUM_QT_CBF_CTX + ctx]);
}

void TEncSbac::codeQtRootCbfZero(TComDataCU*)
{
    // this function is only used to estimate the bits when cbf is 0
    // and will never be called when writing the bistream. do not need to write log
    UInt cbf = 0;
    UInt ctx = 0;

    m_binIf->encodeBin(cbf, m_contextModels[OFF_QT_ROOT_CBF_CTX + ctx]);
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
    ContextModel *ctxX = &m_contextModels[OFF_CTX_LAST_FLAG_X + (ttype ? NUM_CTX_LAST_FLAG_XY : 0)];
    ContextModel *ctxY = &m_contextModels[OFF_CTX_LAST_FLAG_Y + (ttype ? NUM_CTX_LAST_FLAG_XY : 0)];
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
#else // if ENC_DEC_TRACE
    (void)depth;
#endif // if ENC_DEC_TRACE

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
    ttype = ttype == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA;

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
    ContextModel * const baseCoeffGroupCtx = &m_contextModels[OFF_SIG_CG_FLAG_CTX + (ttype ? NUM_SIG_CG_FLAG_CTX : 0)];
    ContextModel * const baseCtx = (ttype == TEXT_LUMA) ? &m_contextModels[OFF_SIG_FLAG_CTX] : &m_contextModels[OFF_SIG_FLAG_CTX + NUM_SIG_FLAG_CTX_LUMA];

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
            ContextModel *baseCtxMod = (ttype == TEXT_LUMA) ? &m_contextModels[OFF_ONE_FLAG_CTX + 4 * ctxSet] : &m_contextModels[OFF_ONE_FLAG_CTX + NUM_ONE_FLAG_CTX_LUMA + 4 * ctxSet];

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
                baseCtxMod = (ttype == TEXT_LUMA) ? &m_contextModels[OFF_ABS_FLAG_CTX + ctxSet] : &m_contextModels[OFF_ABS_FLAG_CTX + NUM_ABS_FLAG_CTX_LUMA + ctxSet];
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

void TEncSbac::codeSaoMaxUvlc(UInt code, UInt maxSymbol)
{
    assert(maxSymbol > 0);

    UInt isCodeLast = (maxSymbol > code) ? 1 : 0;
    UInt isCodeNonZero = (code != 0) ? 1 : 0;

    m_binIf->encodeBinEP(isCodeNonZero);
    if (isCodeNonZero)
    {
        UInt mask = (1 << (code - 1)) - 1;
        UInt len = code - 1 + isCodeLast;
        mask <<= isCodeLast;

        m_binIf->encodeBinsEP(mask, len);
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
    assert((code == 0) || (code == 1));
    m_binIf->encodeBin(code, m_contextModels[OFF_SAO_MERGE_FLAG_CTX]);
}

/** Code SAO type index
 * \param code
 */
void TEncSbac::codeSaoTypeIdx(UInt code)
{
    m_binIf->encodeBin((code == 0) ? 0 : 1, m_contextModels[OFF_SAO_TYPE_IDX_CTX]);
    if (code != 0)
    {
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
    ContextModel *ctx = &m_contextModels[OFF_QT_CBF_CTX];

    for (UInt uiCtxInc = 0; uiCtxInc < 3 * NUM_QT_CBF_CTX; uiCtxInc++)
    {
        estBitsSbac->blockCbpBits[uiCtxInc][0] = ctx[uiCtxInc].getEntropyBits(0);
        estBitsSbac->blockCbpBits[uiCtxInc][1] = ctx[uiCtxInc].getEntropyBits(1);
    }

    ctx = &m_contextModels[OFF_QT_ROOT_CBF_CTX];

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
            estBitsSbac->significantCoeffGroupBits[ctxIdx][bin] = m_contextModels[OFF_SIG_CG_FLAG_CTX + ((ttype ? NUM_SIG_CG_FLAG_CTX : 0) + ctxIdx)].getEntropyBits(bin);
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
            estBitsSbac->significantBits[0][bin] = m_contextModels[OFF_SIG_FLAG_CTX].getEntropyBits(bin);
        }

        for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
        {
            for (UInt bin = 0; bin < 2; bin++)
            {
                estBitsSbac->significantBits[ctxIdx][bin] = m_contextModels[OFF_SIG_FLAG_CTX + ctxIdx].getEntropyBits(bin);
            }
        }
    }
    else
    {
        for (UInt bin = 0; bin < 2; bin++)
        {
            estBitsSbac->significantBits[0][bin] = m_contextModels[OFF_SIG_FLAG_CTX + (NUM_SIG_FLAG_CTX_LUMA + 0)].getEntropyBits(bin);
        }

        for (int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
        {
            for (UInt bin = 0; bin < 2; bin++)
            {
                estBitsSbac->significantBits[ctxIdx][bin] = m_contextModels[OFF_SIG_FLAG_CTX + (NUM_SIG_FLAG_CTX_LUMA + ctxIdx)].getEntropyBits(bin);
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
        estBitsSbac->lastXBits[ctx] = bitsX + m_contextModels[OFF_CTX_LAST_FLAG_X + ((ttype ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset)].getEntropyBits(0);
        bitsX += m_contextModels[OFF_CTX_LAST_FLAG_X + ((ttype ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset)].getEntropyBits(1);
    }

    estBitsSbac->lastXBits[ctx] = bitsX;
    for (ctx = 0; ctx < g_groupIdx[height - 1]; ctx++)
    {
        int ctxOffset = blkSizeOffsetY + (ctx >> shiftY);
        estBitsSbac->lastYBits[ctx] = bitsY + m_contextModels[OFF_CTX_LAST_FLAG_Y + ((ttype ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset)].getEntropyBits(0);
        bitsY += m_contextModels[OFF_CTX_LAST_FLAG_Y + ((ttype ? NUM_CTX_LAST_FLAG_XY : 0) + ctxOffset)].getEntropyBits(1);
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
        ContextModel *ctxOne = &m_contextModels[OFF_ONE_FLAG_CTX];
        ContextModel *ctxAbs = &m_contextModels[OFF_ABS_FLAG_CTX];

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
        ContextModel *ctxOne = &m_contextModels[OFF_ONE_FLAG_CTX + NUM_ONE_FLAG_CTX_LUMA];
        ContextModel *ctxAbs = &m_contextModels[OFF_ABS_FLAG_CTX + NUM_ABS_FLAG_CTX_LUMA];

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
    memcpy(m_contextModels, src->m_contextModels, MAX_OFF_CTX_MOD * sizeof(m_contextModels[0]));
}

void  TEncSbac::loadContexts(TEncSbac* src)
{
    this->xCopyContextsFrom(src);
}

//! \}
