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

/** \file     TEncCu.cpp
    \brief    Coding Unit (CU) encoder class
*/

#include "TEncCu.h"

#include "primitives.h"
#include "common.h"

#include "rdcost.h"
#include "encoder.h"

#include "PPA/ppa.h"

using namespace x265;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCu::TEncCu()
{
    m_interCU_2Nx2N   = NULL;
    m_interCU_2NxN    = NULL;
    m_interCU_Nx2N    = NULL;
    m_intraInInterCU  = NULL;
    m_mergeCU         = NULL;
    m_bestMergeCU     = NULL;
    m_bestCU          = NULL;
    m_tempCU          = NULL;

    m_bestPredYuv     = NULL;
    m_bestResiYuv     = NULL;
    m_bestRecoYuv     = NULL;

    m_tmpPredYuv      = NULL;
    m_tmpResiYuv      = NULL;
    m_tmpRecoYuv      = NULL;
    m_bestMergeRecoYuv = NULL;
    m_origYuv         = NULL;
    for (int i = 0; i < MAX_PRED_TYPES; i++)
    {
        m_modePredYuv[i] = NULL;
    }

    m_search          = NULL;
    m_trQuant         = NULL;
    m_rdCost          = NULL;
    m_bitCounter      = NULL;
    m_entropyCoder    = NULL;
    m_rdSbacCoders    = NULL;
    m_rdGoOnSbacCoder = NULL;
}

/**
 \param    totalDepth  total number of allowable depth
 \param    maxWidth    largest CU width
 \param    maxHeight   largest CU height
 */
bool TEncCu::create(uint8_t totalDepth, uint32_t maxWidth)
{
    m_totalDepth     = totalDepth + 1;
    m_interCU_2Nx2N  = new TComDataCU*[m_totalDepth - 1];
    m_interCU_2NxN   = new TComDataCU*[m_totalDepth - 1];
    m_interCU_Nx2N   = new TComDataCU*[m_totalDepth - 1];
    m_intraInInterCU = new TComDataCU*[m_totalDepth - 1];
    m_mergeCU        = new TComDataCU*[m_totalDepth - 1];
    m_bestMergeCU    = new TComDataCU*[m_totalDepth - 1];
    m_bestCU         = new TComDataCU*[m_totalDepth - 1];
    m_tempCU         = new TComDataCU*[m_totalDepth - 1];

    m_bestPredYuv = new TComYuv*[m_totalDepth - 1];
    m_bestResiYuv = new ShortYuv*[m_totalDepth - 1];
    m_bestRecoYuv = new TComYuv*[m_totalDepth - 1];

    m_tmpPredYuv = new TComYuv*[m_totalDepth - 1];

    m_modePredYuv[0] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[1] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[2] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[3] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[4] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[5] = new TComYuv*[m_totalDepth - 1];

    m_tmpResiYuv = new ShortYuv*[m_totalDepth - 1];
    m_tmpRecoYuv = new TComYuv*[m_totalDepth - 1];

    m_bestMergeRecoYuv = new TComYuv*[m_totalDepth - 1];

    m_origYuv = new TComYuv*[m_totalDepth - 1];

    int unitSize = maxWidth >> (m_totalDepth - 1);
    int csp = m_param->internalCsp;

    bool ok = true;
    for (int i = 0; i < m_totalDepth - 1; i++)
    {
        uint32_t numPartitions = 1 << ((m_totalDepth - i - 1) << 1);
        uint32_t cuSize = maxWidth >> i;

        m_bestCU[i] = new TComDataCU;
        ok &= m_bestCU[i]->create(numPartitions, cuSize, unitSize, csp);

        m_tempCU[i] = new TComDataCU;
        ok &= m_tempCU[i]->create(numPartitions, cuSize, unitSize, csp);

        m_interCU_2Nx2N[i] = new TComDataCU;
        ok &= m_interCU_2Nx2N[i]->create(numPartitions, cuSize, unitSize, csp);

        m_interCU_2NxN[i] = new TComDataCU;
        ok &= m_interCU_2NxN[i]->create(numPartitions, cuSize, unitSize, csp);

        m_interCU_Nx2N[i] = new TComDataCU;
        ok &= m_interCU_Nx2N[i]->create(numPartitions, cuSize, unitSize, csp);

        m_intraInInterCU[i] = new TComDataCU;
        ok &= m_intraInInterCU[i]->create(numPartitions, cuSize, unitSize, csp);

        m_mergeCU[i] = new TComDataCU;
        ok &= m_mergeCU[i]->create(numPartitions, cuSize, unitSize, csp);

        m_bestMergeCU[i] = new TComDataCU;
        ok &= m_bestMergeCU[i]->create(numPartitions, cuSize, unitSize, csp);

        m_bestPredYuv[i] = new TComYuv;
        ok &= m_bestPredYuv[i]->create(cuSize, cuSize, csp);

        m_bestResiYuv[i] = new ShortYuv;
        ok &= m_bestResiYuv[i]->create(cuSize, cuSize, csp);

        m_bestRecoYuv[i] = new TComYuv;
        ok &= m_bestRecoYuv[i]->create(cuSize, cuSize, csp);

        m_tmpPredYuv[i] = new TComYuv;
        ok &= m_tmpPredYuv[i]->create(cuSize, cuSize, csp);

        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            m_modePredYuv[j][i] = new TComYuv;
            ok &= m_modePredYuv[j][i]->create(cuSize, cuSize, csp);
        }

        m_tmpResiYuv[i] = new ShortYuv;
        ok &= m_tmpResiYuv[i]->create(cuSize, cuSize, csp);

        m_tmpRecoYuv[i] = new TComYuv;
        ok &= m_tmpRecoYuv[i]->create(cuSize, cuSize, csp);

        m_bestMergeRecoYuv[i] = new TComYuv;
        ok &= m_bestMergeRecoYuv[i]->create(cuSize, cuSize, csp);

        m_origYuv[i] = new TComYuv;
        ok &= m_origYuv[i]->create(cuSize, cuSize, csp);
    }

    m_bEncodeDQP = false;
    return ok;
}

void TEncCu::destroy()
{
    for (int i = 0; i < m_totalDepth - 1; i++)
    {
        if (m_interCU_2Nx2N && m_interCU_2Nx2N[i])
        {
            m_interCU_2Nx2N[i]->destroy();
            delete m_interCU_2Nx2N[i];
            m_interCU_2Nx2N[i] = NULL;
        }
        if (m_interCU_2NxN && m_interCU_2NxN[i])
        {
            m_interCU_2NxN[i]->destroy();
            delete m_interCU_2NxN[i];
            m_interCU_2NxN[i] = NULL;
        }
        if (m_interCU_Nx2N && m_interCU_Nx2N[i])
        {
            m_interCU_Nx2N[i]->destroy();
            delete m_interCU_Nx2N[i];
            m_interCU_Nx2N[i] = NULL;
        }
        if (m_intraInInterCU && m_intraInInterCU[i])
        {
            m_intraInInterCU[i]->destroy();
            delete m_intraInInterCU[i];
            m_intraInInterCU[i] = NULL;
        }
        if (m_mergeCU && m_mergeCU[i])
        {
            m_mergeCU[i]->destroy();
            delete m_mergeCU[i];
            m_mergeCU[i] = NULL;
        }
        if (m_bestMergeCU && m_bestMergeCU[i])
        {
            m_bestMergeCU[i]->destroy();
            delete m_bestMergeCU[i];
            m_bestMergeCU[i] = NULL;
        }
        if (m_bestCU && m_bestCU[i])
        {
            m_bestCU[i]->destroy();
            delete m_bestCU[i];
            m_bestCU[i] = NULL;
        }
        if (m_tempCU && m_tempCU[i])
        {
            m_tempCU[i]->destroy();
            delete m_tempCU[i];
            m_tempCU[i] = NULL;
        }

        if (m_bestPredYuv && m_bestPredYuv[i])
        {
            m_bestPredYuv[i]->destroy();
            delete m_bestPredYuv[i];
            m_bestPredYuv[i] = NULL;
        }
        if (m_bestResiYuv && m_bestResiYuv[i])
        {
            m_bestResiYuv[i]->destroy();
            delete m_bestResiYuv[i];
            m_bestResiYuv[i] = NULL;
        }
        if (m_bestRecoYuv && m_bestRecoYuv[i])
        {
            m_bestRecoYuv[i]->destroy();
            delete m_bestRecoYuv[i];
            m_bestRecoYuv[i] = NULL;
        }

        if (m_tmpPredYuv && m_tmpPredYuv[i])
        {
            m_tmpPredYuv[i]->destroy();
            delete m_tmpPredYuv[i];
            m_tmpPredYuv[i] = NULL;
        }
        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            if (m_modePredYuv[j] && m_modePredYuv[j][i])
            {
                m_modePredYuv[j][i]->destroy();
                delete m_modePredYuv[j][i];
                m_modePredYuv[j][i] = NULL;
            }
        }

        if (m_tmpResiYuv && m_tmpResiYuv[i])
        {
            m_tmpResiYuv[i]->destroy();
            delete m_tmpResiYuv[i];
            m_tmpResiYuv[i] = NULL;
        }
        if (m_tmpRecoYuv && m_tmpRecoYuv[i])
        {
            m_tmpRecoYuv[i]->destroy();
            delete m_tmpRecoYuv[i];
            m_tmpRecoYuv[i] = NULL;
        }
        if (m_bestMergeRecoYuv && m_bestMergeRecoYuv[i])
        {
            m_bestMergeRecoYuv[i]->destroy();
            delete m_bestMergeRecoYuv[i];
            m_bestMergeRecoYuv[i] = NULL;
        }

        if (m_origYuv && m_origYuv[i])
        {
            m_origYuv[i]->destroy();
            delete m_origYuv[i];
            m_origYuv[i] = NULL;
        }
    }

    delete [] m_interCU_2Nx2N;
    m_interCU_2Nx2N = NULL;
    delete [] m_interCU_2NxN;
    m_interCU_2NxN = NULL;
    delete [] m_interCU_Nx2N;
    m_interCU_Nx2N = NULL;
    delete [] m_intraInInterCU;
    m_intraInInterCU = NULL;
    delete [] m_mergeCU;
    m_mergeCU = NULL;
    delete [] m_bestMergeCU;
    m_bestMergeCU = NULL;
    delete [] m_bestCU;
    m_bestCU = NULL;
    delete [] m_tempCU;
    m_tempCU = NULL;

    delete [] m_bestPredYuv;
    m_bestPredYuv = NULL;
    delete [] m_bestResiYuv;
    m_bestResiYuv = NULL;
    delete [] m_bestRecoYuv;
    m_bestRecoYuv = NULL;

    delete [] m_bestMergeRecoYuv;
    m_bestMergeRecoYuv = NULL;
    delete [] m_tmpPredYuv;
    m_tmpPredYuv = NULL;

    for (int i = 0; i < MAX_PRED_TYPES; i++)
    {
        delete [] m_modePredYuv[i];
        m_modePredYuv[i] = NULL;
    }

    delete [] m_tmpResiYuv;
    m_tmpResiYuv = NULL;
    delete [] m_tmpRecoYuv;
    m_tmpRecoYuv = NULL;
    delete [] m_origYuv;
    m_origYuv = NULL;
}

/** \param    pcEncTop      pointer of encoder class
 */
void TEncCu::init(Encoder* top)
{
    m_param = top->param;
    m_CUTransquantBypassFlagValue = top->m_CUTransquantBypassFlagValue;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  rpcCU pointer of CU data class
 */

void TEncCu::compressCU(TComDataCU* cu)
{
    if (cu->getSlice()->getPPS()->getUseDQP())
    {
        setdQPFlag(true);
    }
    // initialize CU data
    m_bestCU[0]->initCU(cu->getPic(), cu->getAddr());
    m_tempCU[0]->initCU(cu->getPic(), cu->getAddr());

    // analysis of CU
#if LOG_CU_STATISTICS
    int numPartition = cu->getTotalNumPart();
#endif

    if (m_bestCU[0]->getSlice()->getSliceType() == I_SLICE)
    {
        xCompressIntraCU(m_bestCU[0], m_tempCU[0], 0, false);
#if LOG_CU_STATISTICS
        int i = 0, part;
        do
        {
            m_log->totalCu++;
            part = cu->getDepth(i);
            int next = numPartition >> (part * 2);
            if (part == g_maxCUDepth - 1 && cu->getPartitionSize(i) != SIZE_2Nx2N)
            {
                m_log->cntIntraNxN++;
            }
            else
            {
                m_log->cntIntra[part]++;
                if (cu->getLumaIntraDir()[i] > 1)
                    m_log->cuIntraDistribution[part][ANGULAR_MODE_ID]++;
                else
                    m_log->cuIntraDistribution[part][cu->getLumaIntraDir()[i]]++;
            }
            i += next;
        }
        while (i < numPartition);
#endif // if LOG_CU_STATISTICS
    }
    else
    {
        if (m_param->rdLevel < 5)
        {
            TComDataCU* outBestCU = NULL;

            /* At the start of analysis, the best CU is a null pointer
            On return, it points to the CU encode with best chosen mode*/
            xCompressInterCU(outBestCU, m_tempCU[0], cu, 0, false, 0, 4);
        }
        else
            xCompressCU(m_bestCU[0], m_tempCU[0], 0, false);
#if LOG_CU_STATISTICS
        int i = 0, part;
        do
        {
            part = cu->getDepth(i);
            m_log->cntTotalCu[part]++;
            int next = numPartition >> (part * 2);
            if (cu->isSkipped(i))
            {
                m_log->cntSkipCu[part]++;
            }
            else
            {
                m_log->totalCu++;
                if (cu->getPredictionMode(0) == MODE_INTER)
                {
                    m_log->cntInter[part]++;
                    if (cu->getPartitionSize(0) < AMP_ID)
                        m_log->cuInterDistribution[part][cu->getPartitionSize(0)]++;
                    else
                        m_log->cuInterDistribution[part][AMP_ID]++;
                }
                else if (cu->getPredictionMode(0) == MODE_INTRA)
                {
                    if (part == g_maxCUDepth - 1 && cu->getPartitionSize(0) == SIZE_NxN)
                    {
                        m_log->cntIntraNxN++;
                    }
                    else
                    {
                        m_log->cntIntra[part]++;
                        if (cu->getLumaIntraDir()[0] > 1)
                            m_log->cuIntraDistribution[part][ANGULAR_MODE_ID]++;
                        else
                            m_log->cuIntraDistribution[part][cu->getLumaIntraDir()[0]]++;
                    }
                }
            }
            i = i + next;
        }
        while (i < numPartition);
#endif // if LOG_CU_STATISTICS
    }
}

/** \param  cu  pointer of CU data class
 */
void TEncCu::encodeCU(TComDataCU* cu)
{
    if (cu->getSlice()->getPPS()->getUseDQP())
    {
        setdQPFlag(true);
    }

    // Encode CU data
    xEncodeCU(cu, 0, 0, false);
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** Derive small set of test modes for AMP encoder speed-up
 *\param   outBestCU
 *\param   parentSize
 *\param   bTestAMP_Hor
 *\param   bTestAMP_Ver
 *\param   bTestMergeAMP_Hor
 *\param   bTestMergeAMP_Ver
 *\returns void
*/
void TEncCu::deriveTestModeAMP(TComDataCU* outBestCU, PartSize parentSize, bool &bTestAMP_Hor, bool &bTestAMP_Ver,
                               bool &bTestMergeAMP_Hor, bool &bTestMergeAMP_Ver)
{
    if (outBestCU->getPartitionSize(0) == SIZE_2NxN)
    {
        bTestAMP_Hor = true;
    }
    else if (outBestCU->getPartitionSize(0) == SIZE_Nx2N)
    {
        bTestAMP_Ver = true;
    }
    else if (outBestCU->getPartitionSize(0) == SIZE_2Nx2N && outBestCU->getMergeFlag(0) == false &&
             outBestCU->isSkipped(0) == false)
    {
        bTestAMP_Hor = true;
        bTestAMP_Ver = true;
    }

    //! Utilizing the partition size of parent PU
    if (parentSize >= SIZE_2NxnU && parentSize <= SIZE_nRx2N)
    {
        bTestMergeAMP_Hor = true;
        bTestMergeAMP_Ver = true;
    }

    if (parentSize == SIZE_NONE) //! if parent is intra
    {
        if (outBestCU->getPartitionSize(0) == SIZE_2NxN)
        {
            bTestMergeAMP_Hor = true;
        }
        else if (outBestCU->getPartitionSize(0) == SIZE_Nx2N)
        {
            bTestMergeAMP_Ver = true;
        }
    }

    if (outBestCU->getPartitionSize(0) == SIZE_2Nx2N && outBestCU->isSkipped(0) == false)
    {
        bTestMergeAMP_Hor = true;
        bTestMergeAMP_Ver = true;
    }

    if (outBestCU->getCUSize(0) == 64)
    {
        bTestAMP_Hor = false;
        bTestAMP_Ver = false;
    }
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** Compress a CU block recursively with enabling sub-LCU-level delta QP
 *\param   outBestCU
 *\param   outTempCU
 *\param   depth
 *\returns void
 *
 *- for loop of QP value to compress the current CU with all possible QP
*/

void TEncCu::xCompressIntraCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, bool bInsidePicture)
{
    //PPAScopeEvent(TEncCu_xCompressIntraCU + depth);

    TComPic* pic = outBestCU->getPic();

    if (depth == 0)
    {
        // get original YUV data from picture
        m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU());
    }
    else
    {
        // copy partition YUV from depth 0 CTU cache
        m_origYuv[0]->copyPartToYuv(m_origYuv[depth], outBestCU->getZorderIdxInCU());
    }

    uint32_t cuSize = outTempCU->getCUSize(0);
    TComSlice* slice = outTempCU->getSlice();
    if (!bInsidePicture)
    {
        uint32_t lpelx = outBestCU->getCUPelX();
        uint32_t tpely = outBestCU->getCUPelY();
        uint32_t rpelx = lpelx + cuSize;
        uint32_t bpely = tpely + cuSize;
        bInsidePicture = (rpelx <= slice->getSPS()->getPicWidthInLumaSamples() &&
                          bpely <= slice->getSPS()->getPicHeightInLumaSamples());
    }

    // We need to split, so don't try these modes.
    if (bInsidePicture)
    {
        outTempCU->initEstData(depth);

        xCheckRDCostIntra(outBestCU, outTempCU, SIZE_2Nx2N);
        outTempCU->initEstData(depth);

        if (depth == g_maxCUDepth - g_addCUDepth)
        {
            if (cuSize > (1 << slice->getSPS()->getQuadtreeTULog2MinSize()))
            {
                xCheckRDCostIntra(outBestCU, outTempCU, SIZE_NxN);
            }
        }

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth);
        outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        outBestCU->m_totalRDCost  = m_rdCost->calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);
    }

    outTempCU->initEstData(depth);

    // further split
    if (depth < g_maxCUDepth - g_addCUDepth)
    {
        uint8_t     nextDepth     = depth + 1;
        TComDataCU* subBestPartCU = m_bestCU[nextDepth];
        TComDataCU* subTempPartCU = m_tempCU[nextDepth];
        uint32_t partUnitIdx = 0;
        for (; partUnitIdx < 4; partUnitIdx++)
        {
            subBestPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth); // clear sub partition datas or init.

            if (bInsidePicture ||
                ((subBestPartCU->getCUPelX() < slice->getSPS()->getPicWidthInLumaSamples()) &&
                 (subBestPartCU->getCUPelY() < slice->getSPS()->getPicHeightInLumaSamples())))
            {
                subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth); // clear sub partition datas or init.
                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[nextDepth][CI_NEXT_BEST]);
                }

                xCompressIntraCU(subBestPartCU, subTempPartCU, nextDepth, bInsidePicture);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(subBestPartCU->getTotalNumPart() * partUnitIdx, nextDepth);
            }
            else
            {
                subBestPartCU->copyToPic(nextDepth);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth);
            }
        }

        if (bInsidePicture)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(outTempCU, 0, depth);
            outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        }
        outTempCU->m_totalRDCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        if ((g_maxCUSize >> depth) == slice->getPPS()->getMinCuDQPSize() && slice->getPPS()->getUseDQP())
        {
            bool hasResidual = false;
            for (uint32_t blkIdx = 0; blkIdx < outTempCU->getTotalNumPart(); blkIdx++)
            {
                if (outTempCU->getCbf(blkIdx, TEXT_LUMA) || outTempCU->getCbf(blkIdx, TEXT_CHROMA_U) ||
                    outTempCU->getCbf(blkIdx, TEXT_CHROMA_V))
                {
                    hasResidual = true;
                    break;
                }
            }

            uint32_t targetPartIdx = 0;
            if (hasResidual)
            {
                bool foundNonZeroCbf = false;
                outTempCU->setQPSubCUs(outTempCU->getRefQP(targetPartIdx), outTempCU, 0, depth, foundNonZeroCbf);
                X265_CHECK(foundNonZeroCbf, "expected to find non-zero CBF\n");
            }
            else
            {
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
            }
        }

        m_rdSbacCoders[nextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
        xCheckBestMode(outBestCU, outTempCU, depth); // RD compare current CU against split
    }
    outBestCU->copyToPic(depth); // Copy Best data to Picture for next partition prediction.

    if (!bInsidePicture) return;

    // Copy Yuv data to picture Yuv
    xCopyYuv2Pic(pic, outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth);

    X265_CHECK(outBestCU->getPartitionSize(0) != SIZE_NONE, "no best partition size\n");
    X265_CHECK(outBestCU->getPredictionMode(0) != MODE_NONE, "no best partition mode\n");
    X265_CHECK(outBestCU->m_totalRDCost != MAX_INT64, "no best partition cost\n");
}

void TEncCu::xCompressCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, bool bInsidePicture, PartSize parentSize)
{
    //PPAScopeEvent(TEncCu_xCompressCU + depth);

    TComPic* pic = outBestCU->getPic();

    if (depth == 0)
    {
        // get original YUV data from picture
        m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU());
    }
    else
    {
        // copy partition YUV from depth 0 CTU cache
        m_origYuv[0]->copyPartToYuv(m_origYuv[depth], outBestCU->getZorderIdxInCU());
    }

    // variable for Early CU determination
    bool bSubBranch = true;

    // variable for Cbf fast mode PU decision
    bool doNotBlockPu = true;
    bool earlyDetectionSkipMode = false;

    uint32_t cuSize = outTempCU->getCUSize(0);
    TComSlice* slice = outTempCU->getSlice();
    if (!bInsidePicture)
    {
        uint32_t lpelx = outBestCU->getCUPelX();
        uint32_t tpely = outBestCU->getCUPelY();
        uint32_t rpelx = lpelx + cuSize;
        uint32_t bpely = tpely + cuSize;
        bInsidePicture = (rpelx <= slice->getSPS()->getPicWidthInLumaSamples() &&
                          bpely <= slice->getSPS()->getPicHeightInLumaSamples());
    }

    // We need to split, so don't try these modes.
    if (bInsidePicture)
    {
        outTempCU->initEstData(depth);

        // do inter modes, SKIP and 2Nx2N
        if (slice->getSliceType() != I_SLICE)
        {
            // 2Nx2N
            if (m_param->bEnableEarlySkip)
            {
                xCheckRDCostInter(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData(depth); // by competition for inter_2Nx2N
            }
            // by Merge for inter_2Nx2N
            xCheckRDCostMerge2Nx2N(outBestCU, outTempCU, &earlyDetectionSkipMode, m_bestPredYuv[depth], m_bestRecoYuv[depth]);

            outTempCU->initEstData(depth);

            if (!m_param->bEnableEarlySkip)
            {
                // 2Nx2N, NxN
                xCheckRDCostInter(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData(depth);
                if (m_param->bEnableCbfFastMode)
                {
                    doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                }
            }
        }

        if (!earlyDetectionSkipMode)
        {
            outTempCU->initEstData(depth);

            // do inter modes, NxN, 2NxN, and Nx2N
            if (slice->getSliceType() != I_SLICE)
            {
                // 2Nx2N, NxN
                if (!(cuSize == 8))
                {
                    if (depth == g_maxCUDepth - g_addCUDepth && doNotBlockPu)
                    {
                        xCheckRDCostInter(outBestCU, outTempCU, SIZE_NxN);
                        outTempCU->initEstData(depth);
                    }
                }

                if (m_param->bEnableRectInter)
                {
                    // 2NxN, Nx2N
                    if (doNotBlockPu)
                    {
                        xCheckRDCostInter(outBestCU, outTempCU, SIZE_Nx2N);
                        outTempCU->initEstData(depth);
                        if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_Nx2N)
                        {
                            doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                    }
                    if (doNotBlockPu)
                    {
                        xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxN);
                        outTempCU->initEstData(depth);
                        if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxN)
                        {
                            doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                    }
                }

                // Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
                if (slice->getSPS()->getAMPAcc(depth))
                {
                    bool bTestAMP_Hor = false, bTestAMP_Ver = false;
                    bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

                    deriveTestModeAMP(outBestCU, parentSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);

                    // Do horizontal AMP
                    if (bTestAMP_Hor)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnU);
                            outTempCU->initEstData(depth);
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnD);
                            outTempCU->initEstData(depth);
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                    }
                    else if (bTestMergeAMP_Hor)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnU, true);
                            outTempCU->initEstData(depth);
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnD, true);
                            outTempCU->initEstData(depth);
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                    }

                    // Do horizontal AMP
                    if (bTestAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nLx2N);
                            outTempCU->initEstData(depth);
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nRx2N);
                            outTempCU->initEstData(depth);
                        }
                    }
                    else if (bTestMergeAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nLx2N, true);
                            outTempCU->initEstData(depth);
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nRx2N, true);
                            outTempCU->initEstData(depth);
                        }
                    }
                }
            }

            // do normal intra modes
            // speedup for inter frames
            if ((slice->getSliceType() == I_SLICE ||
                outBestCU->getCbf(0, TEXT_LUMA) != 0   ||
                outBestCU->getCbf(0, TEXT_CHROMA_U) != 0   ||
                outBestCU->getCbf(0, TEXT_CHROMA_V) != 0) && m_param->bIntraInBFrames) // avoid very complex intra if it is unlikely
            {
                xCheckRDCostIntraInInter(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData(depth);

                if (depth == g_maxCUDepth - g_addCUDepth)
                {
                    if (cuSize > (1 << slice->getSPS()->getQuadtreeTULog2MinSize()))
                    {
                        xCheckRDCostIntraInInter(outBestCU, outTempCU, SIZE_NxN);
                        outTempCU->initEstData(depth);
                    }
                }
            }
            // test PCM
            if (slice->getSPS()->getUsePCM()
                && cuSize <= (1 << slice->getSPS()->getPCMLog2MaxSize())
                && cuSize >= (1 << slice->getSPS()->getPCMLog2MinSize()))
            {
                uint32_t rawbits = (2 * X265_DEPTH + X265_DEPTH) * cuSize * cuSize / 2;
                uint32_t bestbits = outBestCU->m_totalBits;
                if ((bestbits > rawbits) || (outBestCU->m_totalRDCost > m_rdCost->calcRdCost(0, rawbits)))
                {
                    xCheckIntraPCM(outBestCU, outTempCU);
                }
            }
        }

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth);
        outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        outBestCU->m_totalRDCost  = m_rdCost->calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);

        // Early CU determination
        if (outBestCU->isSkipped(0))
            bSubBranch = false;
        else
            bSubBranch = true;
    }

    // copy original YUV samples to PCM buffer
    if (outBestCU->isLosslessCoded(0) && (outBestCU->getIPCMFlag(0) == false))
    {
        xFillPCMBuffer(outBestCU, m_origYuv[depth]);
    }

    outTempCU->initEstData(depth);

    // further split
    if (bSubBranch && depth < g_maxCUDepth - g_addCUDepth)
    {
        uint8_t     nextDepth     = depth + 1;
        TComDataCU* subBestPartCU = m_bestCU[nextDepth];
        TComDataCU* subTempPartCU = m_tempCU[nextDepth];
        uint32_t partUnitIdx = 0;
        for (; partUnitIdx < 4; partUnitIdx++)
        {
            subBestPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth); // clear sub partition datas or init.

            if (bInsidePicture ||
                ((subBestPartCU->getCUPelX() < slice->getSPS()->getPicWidthInLumaSamples()) &&
                 (subBestPartCU->getCUPelY() < slice->getSPS()->getPicHeightInLumaSamples())))
            {
                subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth); // clear sub partition datas or init.
                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[nextDepth][CI_NEXT_BEST]);
                }

                xCompressCU(subBestPartCU, subTempPartCU, nextDepth, bInsidePicture);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(subBestPartCU->getTotalNumPart() * partUnitIdx, nextDepth);
            }
            else
            {
                subBestPartCU->copyToPic(nextDepth);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth);
            }
        }

        if (bInsidePicture)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(outTempCU, 0, depth);
            outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        }
        outTempCU->m_totalRDCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        if ((g_maxCUSize >> depth) == slice->getPPS()->getMinCuDQPSize() && slice->getPPS()->getUseDQP())
        {
            bool hasResidual = false;
            for (uint32_t blkIdx = 0; blkIdx < outTempCU->getTotalNumPart(); blkIdx++)
            {
                if (outTempCU->getCbf(blkIdx, TEXT_LUMA) || outTempCU->getCbf(blkIdx, TEXT_CHROMA_U) ||
                    outTempCU->getCbf(blkIdx, TEXT_CHROMA_V))
                {
                    hasResidual = true;
                    break;
                }
            }

            uint32_t targetPartIdx = 0;
            if (hasResidual)
            {
                bool foundNonZeroCbf = false;
                outTempCU->setQPSubCUs(outTempCU->getRefQP(targetPartIdx), outTempCU, 0, depth, foundNonZeroCbf);
                X265_CHECK(foundNonZeroCbf, "expected to find non-zero CBF\n");
            }
            else
            {
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
            }
        }

        m_rdSbacCoders[nextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
        xCheckBestMode(outBestCU, outTempCU, depth); // RD compare current CU against split
    }
    outBestCU->copyToPic(depth); // Copy Best data to Picture for next partition prediction.

    if (!bInsidePicture) return;

    // Copy Yuv data to picture Yuv
    xCopyYuv2Pic(pic, outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth);

    X265_CHECK(outBestCU->getPartitionSize(0) != SIZE_NONE, "no best partition size\n");
    X265_CHECK(outBestCU->getPredictionMode(0) != MODE_NONE, "no best partition mode\n");
    X265_CHECK(outBestCU->m_totalRDCost != MAX_INT64, "no best partition cost\n");
}

/** finish encoding a cu and handle end-of-slice conditions
 * \param cu
 * \param absPartIdx
 * \param depth
 * \returns void
 */
void TEncCu::finishCU(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)
{
    TComPic* pic = cu->getPic();
    TComSlice* slice = cu->getPic()->getSlice();

    //Calculate end address
    uint32_t cuAddr = cu->getSCUAddr() + absPartIdx;

    uint32_t internalAddress = (slice->getSliceCurEndCUAddr() - 1) % pic->getNumPartInCU();
    uint32_t externalAddress = (slice->getSliceCurEndCUAddr() - 1) / pic->getNumPartInCU();
    uint32_t posx = (externalAddress % pic->getFrameWidthInCU()) * g_maxCUSize + g_rasterToPelX[g_zscanToRaster[internalAddress]];
    uint32_t posy = (externalAddress / pic->getFrameWidthInCU()) * g_maxCUSize + g_rasterToPelY[g_zscanToRaster[internalAddress]];
    uint32_t width = slice->getSPS()->getPicWidthInLumaSamples();
    uint32_t height = slice->getSPS()->getPicHeightInLumaSamples();
    uint32_t cuSize = cu->getCUSize(absPartIdx);

    while (posx >= width || posy >= height)
    {
        internalAddress--;
        posx = (externalAddress % pic->getFrameWidthInCU()) * g_maxCUSize + g_rasterToPelX[g_zscanToRaster[internalAddress]];
        posy = (externalAddress / pic->getFrameWidthInCU()) * g_maxCUSize + g_rasterToPelY[g_zscanToRaster[internalAddress]];
    }

    internalAddress++;
    if (internalAddress == cu->getPic()->getNumPartInCU())
    {
        internalAddress = 0;
        externalAddress = (externalAddress + 1);
    }
    uint32_t realEndAddress = (externalAddress * pic->getNumPartInCU() + internalAddress);

    // Encode slice finish
    bool bTerminateSlice = false;
    if (cuAddr + (cu->getPic()->getNumPartInCU() >> (depth << 1)) == realEndAddress)
    {
        bTerminateSlice = true;
    }
    uint32_t uiGranularityWidth = g_maxCUSize;
    posx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    posy = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
    bool granularityBoundary = ((posx + cuSize) % uiGranularityWidth == 0 || (posx + cuSize == width))
        && ((posy + cuSize) % uiGranularityWidth == 0 || (posy + cuSize == height));

    if (granularityBoundary)
    {
        // The 1-terminating bit is added to all streams, so don't add it here when it's 1.
        if (!bTerminateSlice)
            m_entropyCoder->encodeTerminatingBit(bTerminateSlice ? 1 : 0);
    }

    int numberOfWrittenBits = 0;
    if (m_bitCounter)
    {
        numberOfWrittenBits = m_entropyCoder->getNumberOfWrittenBits();
    }

    if (granularityBoundary)
    {
        slice->setSliceBits((uint32_t)(slice->getSliceBits() + numberOfWrittenBits));
        slice->setSliceSegmentBits(slice->getSliceSegmentBits() + numberOfWrittenBits);
        if (m_bitCounter)
        {
            m_entropyCoder->resetBits();
        }
    }
}

/** encode a CU block recursively
 * \param cu
 * \param absPartIdx
 * \param depth
 * \returns void
 */
void TEncCu::xEncodeCU(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool bInsidePicture)
{
    TComPic* pic = cu->getPic();

    TComSlice* slice = cu->getSlice();
    if (!bInsidePicture)
    {
        uint32_t lpelx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
        uint32_t tpely = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
        uint32_t rpelx = lpelx + (g_maxCUSize >> depth);
        uint32_t bpely = tpely + (g_maxCUSize >> depth);
        bInsidePicture = (rpelx <= slice->getSPS()->getPicWidthInLumaSamples() &&
                          bpely <= slice->getSPS()->getPicHeightInLumaSamples());
    }

    // We need to split, so don't try these modes.
    if (bInsidePicture)
    {
        m_entropyCoder->encodeSplitFlag(cu, absPartIdx, depth);
    }

    if ((g_maxCUSize >> depth) >= slice->getPPS()->getMinCuDQPSize() && slice->getPPS()->getUseDQP())
    {
        setdQPFlag(true);
    }

    if (!bInsidePicture)
    {
        uint32_t qNumParts = (pic->getNumPartInCU() >> (depth << 1)) >> 2;

        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++, absPartIdx += qNumParts)
        {
            uint32_t lpelx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
            uint32_t tpely = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
            if ((lpelx < slice->getSPS()->getPicWidthInLumaSamples()) &&
                (tpely < slice->getSPS()->getPicHeightInLumaSamples()))
            {
                xEncodeCU(cu, absPartIdx, depth + 1, bInsidePicture);
            }
        }

        return;
    }

    if ((depth < cu->getDepth(absPartIdx)) && (depth < (g_maxCUDepth - g_addCUDepth)))
    {
        uint32_t qNumParts = (pic->getNumPartInCU() >> (depth << 1)) >> 2;

        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++, absPartIdx += qNumParts)
        {
            xEncodeCU(cu, absPartIdx, depth + 1, bInsidePicture);
        }

        return;
    }

    if (slice->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(cu, absPartIdx);
    }
    if (!slice->isIntra())
    {
        m_entropyCoder->encodeSkipFlag(cu, absPartIdx);
    }

    if (cu->isSkipped(absPartIdx))
    {
        m_entropyCoder->encodeMergeIndex(cu, absPartIdx);
        finishCU(cu, absPartIdx, depth);
        return;
    }
    m_entropyCoder->encodePredMode(cu, absPartIdx);

    m_entropyCoder->encodePartSize(cu, absPartIdx, depth);

    if (cu->isIntra(absPartIdx) && cu->getPartitionSize(absPartIdx) == SIZE_2Nx2N)
    {
        m_entropyCoder->encodeIPCMInfo(cu, absPartIdx);

        if (cu->getIPCMFlag(absPartIdx))
        {
            // Encode slice finish
            finishCU(cu, absPartIdx, depth);
            return;
        }
    }

    // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
    m_entropyCoder->encodePredInfo(cu, absPartIdx);

    // Encode Coefficients
    bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(cu, absPartIdx, depth, cu->getCUSize(absPartIdx), bCodeDQP);
    setdQPFlag(bCodeDQP);

    // --- write terminating bit ---
    finishCU(cu, absPartIdx, depth);
}

/** check RD costs for a CU block encoded with merge
 * \param outBestCU
 * \param outTempCU
 * \returns void
 */
void TEncCu::xCheckRDCostMerge2Nx2N(TComDataCU*& outBestCU, TComDataCU*& outTempCU, bool *earlyDetectionSkipMode, TComYuv*& outBestPredYuv, TComYuv*& rpcYuvReconBest)
{
    X265_CHECK(outTempCU->getSlice()->getSliceType() != I_SLICE, "I slice not expected\n");
    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
    uint8_t interDirNeighbours[MRG_MAX_NUM_CANDS];
    uint32_t maxNumMergeCand = outTempCU->getSlice()->getMaxNumMergeCand();

    uint8_t depth = outTempCU->getDepth(0);
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
    outTempCU->setCUTransquantBypassSubParts(m_CUTransquantBypassFlagValue, 0, depth);
    outTempCU->getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours, maxNumMergeCand);

    int mergeCandBuffer[MRG_MAX_NUM_CANDS];
    for (uint32_t i = 0; i < maxNumMergeCand; ++i)
    {
        mergeCandBuffer[i] = 0;
    }

    bool bestIsSkip = false;

    uint32_t iteration;
    if (outTempCU->isLosslessCoded(0))
    {
        iteration = 1;
    }
    else
    {
        iteration = 2;
    }

    for (uint32_t noResidual = 0; noResidual < iteration; ++noResidual)
    {
        for (uint32_t mergeCand = 0; mergeCand < maxNumMergeCand; ++mergeCand)
        {
            if (m_param->frameNumThreads > 1 &&
                (mvFieldNeighbours[mergeCand][0].mv.y >= (m_param->searchRange + 1) * 4 ||
                 mvFieldNeighbours[mergeCand][1].mv.y >= (m_param->searchRange + 1) * 4))
            {
                continue;
            }
            if (!(noResidual && mergeCandBuffer[mergeCand] == 1))
            {
                if (!(bestIsSkip && !noResidual))
                {
                    // set MC parameters
                    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth); // interprets depth relative to LCU level
                    outTempCU->setCUTransquantBypassSubParts(m_CUTransquantBypassFlagValue, 0, depth);
                    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
                    outTempCU->setMergeFlag(0, true);
                    outTempCU->setMergeIndex(0, mergeCand);
                    outTempCU->setInterDirSubParts(interDirNeighbours[mergeCand], 0, 0, depth); // interprets depth relative to LCU level
                    outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[mergeCand][0], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level
                    outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[mergeCand][1], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level

                    // do MC
                    m_search->motionCompensation(outTempCU, m_tmpPredYuv[depth], REF_PIC_LIST_X, 0);
                    // estimate residual and encode everything
                    m_search->encodeResAndCalcRdInterCU(outTempCU,
                                                        m_origYuv[depth],
                                                        m_tmpPredYuv[depth],
                                                        m_tmpResiYuv[depth],
                                                        m_bestResiYuv[depth],
                                                        m_tmpRecoYuv[depth],
                                                        !!noResidual,
                                                        true);

                    /* Todo: Fix the satd cost estimates. Why is merge being chosen in high motion areas: estimated distortion is too low? */
                    if (!noResidual && !outTempCU->getQtRootCbf(0))
                        mergeCandBuffer[mergeCand] = 1;

                    outTempCU->setSkipFlagSubParts(!outTempCU->getQtRootCbf(0), 0, depth);
                    int origQP = outTempCU->getQP(0);
                    xCheckDQP(outTempCU);
                    if (outTempCU->m_totalRDCost < outBestCU->m_totalRDCost)
                    {
                        TComDataCU* tmp = outTempCU;
                        outTempCU = outBestCU;
                        outBestCU = tmp;

                        // Change Prediction data
                        TComYuv* yuv = NULL;
                        yuv = outBestPredYuv;
                        outBestPredYuv = m_tmpPredYuv[depth];
                        m_tmpPredYuv[depth] = yuv;

                        yuv = rpcYuvReconBest;
                        rpcYuvReconBest = m_tmpRecoYuv[depth];
                        m_tmpRecoYuv[depth] = yuv;
                    }
                    outTempCU->initEstData(depth, origQP);
                    if (!bestIsSkip)
                    {
                        bestIsSkip = !outBestCU->getQtRootCbf(0);
                    }
                }
            }
        }

        if (noResidual == 0 && m_param->bEnableEarlySkip)
        {
            if (outBestCU->getQtRootCbf(0) == 0)
            {
                if (outBestCU->getMergeFlag(0))
                {
                    *earlyDetectionSkipMode = true;
                }
                else
                {
                    int mvsum = 0;
                    for (uint32_t refListIdx = 0; refListIdx < 2; refListIdx++)
                    {
                        if (outBestCU->getSlice()->getNumRefIdx(refListIdx) > 0)
                        {
                            TComCUMvField* pcCUMvField = outBestCU->getCUMvField(refListIdx);
                            int hor = abs(pcCUMvField->getMvd(0).x);
                            int ver = abs(pcCUMvField->getMvd(0).y);
                            mvsum += hor + ver;
                        }
                    }

                    if (mvsum == 0)
                    {
                        *earlyDetectionSkipMode = true;
                    }
                }
            }
        }
    }
}

void TEncCu::xCheckRDCostInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize, bool bUseMRG)
{
    uint8_t depth = outTempCU->getDepth(0);

    outTempCU->setDepthSubParts(depth);
    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_CUTransquantBypassFlagValue, 0, depth);

    if (m_search->predInterSearch(outTempCU, m_tmpPredYuv[depth], bUseMRG, true))
    {
        m_search->encodeResAndCalcRdInterCU(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_bestResiYuv[depth], m_tmpRecoYuv[depth], false, true);
        xCheckDQP(outTempCU);
        xCheckBestMode(outBestCU, outTempCU, depth);
    }
}

void TEncCu::xCheckRDCostIntra(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize)
{
    //PPAScopeEvent(TEncCU_xCheckRDCostIntra + depth);
    uint32_t depth = outTempCU->getDepth(0);

    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_CUTransquantBypassFlagValue, 0, depth);

    m_search->estIntraPredQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);

    m_search->estIntraPredChromaQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);

    m_entropyCoder->resetBits();
    if (outTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(outTempCU, 0);
    }
    m_entropyCoder->encodeSkipFlag(outTempCU, 0);
    m_entropyCoder->encodePredMode(outTempCU, 0);
    m_entropyCoder->encodePartSize(outTempCU, 0, depth);
    m_entropyCoder->encodePredInfo(outTempCU, 0);
    m_entropyCoder->encodeIPCMInfo(outTempCU, 0);

    // Encode Coefficients
    bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(outTempCU, 0, depth, outTempCU->getCUSize(0), bCodeDQP);
    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
    outTempCU->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();

    if (m_rdCost->psyRdEnabled())
    {
        int part = g_convertToBit[outTempCU->getCUSize(0)];
        uint32_t psyRdCost = m_rdCost->psyCost(part, m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                     m_tmpRecoYuv[depth]->getLumaAddr(), m_tmpRecoYuv[depth]->getStride());
        outTempCU->m_totalRDCost = m_rdCost->calcPsyRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits, psyRdCost);
    }
    else
    {
        outTempCU->m_totalRDCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);
    }
    xCheckDQP(outTempCU);
    xCheckBestMode(outBestCU, outTempCU, depth);
}

void TEncCu::xCheckRDCostIntraInInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize)
{
    uint32_t depth = outTempCU->getDepth(0);

    PPAScopeEvent(TEncCU_xCheckRDCostIntra + depth);

    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_CUTransquantBypassFlagValue, 0, depth);

    m_search->estIntraPredQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);

    m_search->estIntraPredChromaQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);

    m_entropyCoder->resetBits();
    if (outTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(outTempCU, 0);
    }
    m_entropyCoder->encodeSkipFlag(outTempCU, 0);
    m_entropyCoder->encodePredMode(outTempCU, 0);
    m_entropyCoder->encodePartSize(outTempCU, 0, depth);
    m_entropyCoder->encodePredInfo(outTempCU, 0);
    m_entropyCoder->encodeIPCMInfo(outTempCU, 0);

    // Encode Coefficients
    bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(outTempCU, 0, depth, outTempCU->getCUSize(0), bCodeDQP);
    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
    outTempCU->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();

    if (m_rdCost->psyRdEnabled())
    {
        int part = g_convertToBit[outTempCU->getCUSize(0)];
        uint32_t psyRdCost = m_rdCost->psyCost(part, m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                     m_tmpRecoYuv[depth]->getLumaAddr(), m_tmpRecoYuv[depth]->getStride());
        outTempCU->m_totalRDCost = m_rdCost->calcPsyRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits, psyRdCost);
    }
    else
    {
        outTempCU->m_totalRDCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);
    }
    xCheckDQP(outTempCU);
    xCheckBestMode(outBestCU, outTempCU, depth);
}

/** Check R-D costs for a CU with PCM mode.
 * \param outBestCU pointer to best mode CU data structure
 * \param outTempCU pointer to testing mode CU data structure
 * \returns void
 *
 * \note Current PCM implementation encodes sample values in a lossless way. The distortion of PCM mode CUs are zero. PCM mode is selected if the best mode yields bits greater than that of PCM mode.
 */
void TEncCu::xCheckIntraPCM(TComDataCU*& outBestCU, TComDataCU*& outTempCU)
{
    //PPAScopeEvent(TEncCU_xCheckIntraPCM);

    uint32_t depth = outTempCU->getDepth(0);

    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setIPCMFlag(0, true);
    outTempCU->setIPCMFlagSubParts(true, 0, outTempCU->getDepth(0));
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setTrIdxSubParts(0, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_CUTransquantBypassFlagValue, 0, depth);

    m_search->IPCMSearch(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);

    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

    m_entropyCoder->resetBits();
    if (outTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(outTempCU, 0);
    }
    m_entropyCoder->encodeSkipFlag(outTempCU, 0);
    m_entropyCoder->encodePredMode(outTempCU, 0);
    m_entropyCoder->encodePartSize(outTempCU, 0, depth);
    m_entropyCoder->encodeIPCMInfo(outTempCU, 0);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    outTempCU->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();
    outTempCU->m_totalRDCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

    xCheckDQP(outTempCU);
    xCheckBestMode(outBestCU, outTempCU, depth);
}

/** check whether current try is the best with identifying the depth of current try
 * \param outBestCU
 * \param outTempCU
 * \returns void
 */
void TEncCu::xCheckBestMode(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth)
{
    if (outTempCU->m_totalRDCost < outBestCU->m_totalRDCost)
    {
        TComYuv* yuv;
        // Change Information data
        TComDataCU* cu = outBestCU;
        outBestCU = outTempCU;
        outTempCU = cu;

        // Change Prediction data
        yuv = m_bestPredYuv[depth];
        m_bestPredYuv[depth] = m_tmpPredYuv[depth];
        m_tmpPredYuv[depth] = yuv;

        // Change Reconstruction data
        yuv = m_bestRecoYuv[depth];
        m_bestRecoYuv[depth] = m_tmpRecoYuv[depth];
        m_tmpRecoYuv[depth] = yuv;

        m_rdSbacCoders[depth][CI_TEMP_BEST]->store(m_rdSbacCoders[depth][CI_NEXT_BEST]);
    }
}

void TEncCu::xCheckDQP(TComDataCU* cu)
{
    uint32_t depth = cu->getDepth(0);

    if (cu->getSlice()->getPPS()->getUseDQP() && (g_maxCUSize >> depth) >= cu->getSlice()->getPPS()->getMinCuDQPSize())
    {
        if (!cu->getCbf(0, TEXT_LUMA, 0) && !cu->getCbf(0, TEXT_CHROMA_U, 0) && !cu->getCbf(0, TEXT_CHROMA_V, 0))
            cu->setQPSubParts(cu->getRefQP(0), 0, depth); // set QP to default QP
    }
}

void TEncCu::xCopyYuv2Pic(TComPic* outPic, uint32_t cuAddr, uint32_t absPartIdx, uint32_t depth)
{
    m_bestRecoYuv[depth]->copyToPicYuv(outPic->getPicYuvRec(), cuAddr, absPartIdx);
}

void TEncCu::xCopyYuv2Tmp(uint32_t partUnitIdx, uint32_t nextDepth)
{
    m_bestRecoYuv[nextDepth]->copyToPartYuv(m_tmpRecoYuv[nextDepth - 1], partUnitIdx);
}

/** Function for filling the PCM buffer of a CU using its original sample array
 * \param cu pointer to current CU
 * \param fencYuv pointer to original sample array
 * \returns void
 */
void TEncCu::xFillPCMBuffer(TComDataCU* cu, TComYuv* fencYuv)
{
    uint32_t width = cu->getCUSize(0);
    uint32_t height = cu->getCUSize(0);

    pixel* srcY = fencYuv->getLumaAddr();
    pixel* dstY = cu->getPCMSampleY();
    uint32_t srcStride = fencYuv->getStride();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            dstY[x] = srcY[x];
        }

        dstY += width;
        srcY += srcStride;
    }

    pixel* srcCb = fencYuv->getCbAddr();
    pixel* srcCr = fencYuv->getCrAddr();

    pixel* dstCb = cu->getPCMSampleCb();
    pixel* dstCr = cu->getPCMSampleCr();

    uint32_t srcStrideC = fencYuv->getCStride();
    uint32_t heightC = height >> 1;
    uint32_t widthC = width >> 1;

    for (int y = 0; y < heightC; y++)
    {
        for (int x = 0; x < widthC; x++)
        {
            dstCb[x] = srcCb[x];
            dstCr[x] = srcCr[x];
        }

        dstCb += widthC;
        dstCr += widthC;
        srcCb += srcStrideC;
        srcCr += srcStrideC;
    }
}

//! \}
