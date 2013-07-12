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

#include <stdio.h>
#include "TEncTop.h"
#include "TEncCu.h"
#include "TEncAnalyze.h"
#include "TEncPic.h"
#include "PPA/ppa.h"
#include "primitives.h"
#include "common.h"

#include <cmath>
#include <algorithm>
using namespace std;

using namespace x265;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCu::TEncCu()
{
    m_search          = NULL;
    m_trQuant         = NULL;
    m_rdCost          = NULL;
    m_bitCounter      = NULL;
    m_entropyCoder    = NULL;
    m_rdSbacCoders    = NULL;
    m_rdGoOnSbacCoder = NULL;
}

/**
 \param    uiTotalDepth  total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 */
Void TEncCu::create(UChar uhTotalDepth, UInt uiMaxWidth, UInt uiMaxHeight)
{
    m_totalDepth   = uhTotalDepth + 1;
    m_interCU_2Nx2N  = new TComDataCU*[m_totalDepth - 1];
    m_interCU_2NxN   = new TComDataCU*[m_totalDepth - 1];
    m_interCU_Nx2N   = new TComDataCU*[m_totalDepth - 1];
    m_intraInInterCU = new TComDataCU*[m_totalDepth - 1];
    m_mergeCU        = new TComDataCU*[m_totalDepth - 1];
    m_bestMergeCU    = new TComDataCU*[m_totalDepth - 1];
    m_bestCU      = new TComDataCU*[m_totalDepth - 1];
    m_tempCU      = new TComDataCU*[m_totalDepth - 1];

    m_bestPredYuv = new TComYuv*[m_totalDepth - 1];
    m_bestResiYuv = new TShortYUV*[m_totalDepth - 1];
    m_bestRecoYuv = new TComYuv*[m_totalDepth - 1];
    m_tmpPredYuv = new TComYuv*[m_totalDepth - 1];

    m_modePredYuv[0] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[1] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[2] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[3] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[4] = new TComYuv*[m_totalDepth - 1];
    m_modePredYuv[5] = new TComYuv*[m_totalDepth - 1];

    m_tmpResiYuv = new TShortYUV*[m_totalDepth - 1];
    m_tmpRecoYuv = new TComYuv*[m_totalDepth - 1];

    m_bestMergeRecoYuv = new TComYuv*[m_totalDepth - 1];
    
    m_origYuv = new TComYuv*[m_totalDepth - 1];

    for (Int i = 0; i < m_totalDepth - 1; i++)
    {
        UInt uiNumPartitions = 1 << ((m_totalDepth - i - 1) << 1);
        UInt width  = uiMaxWidth  >> i;
        UInt height = uiMaxHeight >> i;

        m_bestCU[i] = new TComDataCU;
        m_bestCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_totalDepth - 1));
        m_tempCU[i] = new TComDataCU;
        m_tempCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_totalDepth - 1));

        m_interCU_2Nx2N[i] = new TComDataCU;
        m_interCU_2Nx2N[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_totalDepth - 1));
        m_interCU_2NxN[i] = new TComDataCU;
        m_interCU_2NxN[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_totalDepth - 1));
        m_interCU_Nx2N[i] = new TComDataCU;
        m_interCU_Nx2N[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_totalDepth - 1));
        m_intraInInterCU[i] = new TComDataCU;
        m_intraInInterCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_totalDepth - 1));
        m_mergeCU[i] = new TComDataCU;
        m_mergeCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_totalDepth - 1));
        m_bestMergeCU[i] = new TComDataCU;
        m_bestMergeCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_totalDepth - 1));
        m_bestPredYuv[i] = new TComYuv;
        m_bestPredYuv[i]->create(width, height);
        m_bestResiYuv[i] = new TShortYUV;
        m_bestResiYuv[i]->create(width, height);
        m_bestRecoYuv[i] = new TComYuv;
        m_bestRecoYuv[i]->create(width, height);

        m_tmpPredYuv[i] = new TComYuv;
        m_tmpPredYuv[i]->create(width, height);

        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            m_modePredYuv[j][i] = new TComYuv;
            m_modePredYuv[j][i]->create(width, height);
        }

        m_tmpResiYuv[i] = new TShortYUV;
        m_tmpResiYuv[i]->create(width, height);

        m_tmpRecoYuv[i] = new TComYuv;
        m_tmpRecoYuv[i]->create(width, height);
                
        m_bestMergeRecoYuv[i] = new TComYuv;
        m_bestMergeRecoYuv[i]->create(width, height);
        
        m_origYuv[i] = new TComYuv;
        m_origYuv[i]->create(width, height);
    }

    m_bEncodeDQP = false;
    m_LCUPredictionSAD = 0;
    m_addSADDepth      = 0;
    m_temporalSAD      = 0;
}

Void TEncCu::destroy()
{
    for (Int i = 0; i < m_totalDepth - 1; i++)
    {
        if (m_interCU_2Nx2N[i])
        {
            m_interCU_2Nx2N[i]->destroy();
            delete m_interCU_2Nx2N[i];
            m_interCU_2Nx2N[i] = NULL;
        }
        if (m_interCU_2NxN[i])
        {
            m_interCU_2NxN[i]->destroy();
            delete m_interCU_2NxN[i];
            m_interCU_2NxN[i] = NULL;
        }
        if (m_interCU_Nx2N[i])
        {
            m_interCU_Nx2N[i]->destroy();
            delete m_interCU_Nx2N[i];
            m_interCU_Nx2N[i] = NULL;
        }
        if (m_intraInInterCU[i])
        {
            m_intraInInterCU[i]->destroy();
            delete m_intraInInterCU[i];
            m_intraInInterCU[i] = NULL;
        }
        if (m_mergeCU[i])
        {
            m_mergeCU[i]->destroy();
            delete m_mergeCU[i];
            m_mergeCU[i] = NULL;
        }
        if (m_bestMergeCU[i])
        {
            m_bestMergeCU[i]->destroy();
            delete m_bestMergeCU[i];
            m_bestMergeCU[i] = NULL;
        }
        if (m_bestCU[i])
        {
            m_bestCU[i]->destroy();
            delete m_bestCU[i];
            m_bestCU[i] = NULL;
        }
        if (m_tempCU[i])
        {
            m_tempCU[i]->destroy();
            delete m_tempCU[i];
            m_tempCU[i] = NULL;
        }
        if (m_bestPredYuv[i])
        {
            m_bestPredYuv[i]->destroy();
            delete m_bestPredYuv[i];
            m_bestPredYuv[i] = NULL;
        }
        if (m_bestResiYuv[i])
        {
            m_bestResiYuv[i]->destroy();
            delete m_bestResiYuv[i];
            m_bestResiYuv[i] = NULL;
        }
        if (m_bestRecoYuv[i])
        {
            m_bestRecoYuv[i]->destroy();
            delete m_bestRecoYuv[i];
            m_bestRecoYuv[i] = NULL;
        }
        if (m_tmpPredYuv[i])
        {
            m_tmpPredYuv[i]->destroy();
            delete m_tmpPredYuv[i];
            m_tmpPredYuv[i] = NULL;
        }
        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            m_modePredYuv[j][i]->destroy();
            delete m_modePredYuv[j][i];
            m_modePredYuv[j][i] = NULL;
        }

        if (m_tmpResiYuv[i])
        {
            m_tmpResiYuv[i]->destroy();
            delete m_tmpResiYuv[i];
            m_tmpResiYuv[i] = NULL;
        }
        if (m_tmpRecoYuv[i])
        {
            m_tmpRecoYuv[i]->destroy();
            delete m_tmpRecoYuv[i];
            m_tmpRecoYuv[i] = NULL;
        }
        if (m_bestMergeRecoYuv[i])
        {
            m_bestMergeRecoYuv[i]->destroy();
            delete m_bestMergeRecoYuv[i];
            m_bestMergeRecoYuv[i] = NULL;
        }

        if (m_origYuv[i])
        {
            m_origYuv[i]->destroy();
            delete m_origYuv[i];
            m_origYuv[i] = NULL;
        }
    }

    if (m_interCU_2Nx2N)
    {
        delete [] m_interCU_2Nx2N;
        m_interCU_2Nx2N = NULL;
    }
    if (m_interCU_2NxN)
    {
        delete [] m_interCU_2NxN;
        m_interCU_2NxN = NULL;
    }
    if (m_interCU_Nx2N)
    {
        delete [] m_interCU_Nx2N;
        m_interCU_Nx2N = NULL;
    }
    if (m_intraInInterCU)
    {
        delete [] m_intraInInterCU;
        m_intraInInterCU = NULL;
    }
    if (m_mergeCU)
    {
        delete [] m_mergeCU;
        m_mergeCU = NULL;
    }
    if (m_bestMergeCU)
    {
        delete [] m_bestMergeCU;
        m_bestMergeCU = NULL;
    }
    if (m_bestCU)
    {
        delete [] m_bestCU;
        m_bestCU = NULL;
    }
    if (m_tempCU)
    {
        delete [] m_tempCU;
        m_tempCU = NULL;
    }

    if (m_bestPredYuv)
    {
        delete [] m_bestPredYuv;
        m_bestPredYuv = NULL;
    }
    if (m_bestResiYuv)
    {
        delete [] m_bestResiYuv;
        m_bestResiYuv = NULL;
    }
    if (m_bestRecoYuv)
    {
        delete [] m_bestRecoYuv;
        m_bestRecoYuv = NULL;
    }
    if (m_bestMergeRecoYuv)
    {
        delete []m_bestMergeRecoYuv;
        m_bestMergeRecoYuv = NULL;
    }
    if (m_tmpPredYuv)
    {
        delete [] m_tmpPredYuv;
        m_tmpPredYuv = NULL;
    }
    for (int i = 0; i < MAX_PRED_TYPES; i++)
    {
        if (m_modePredYuv[i])
        {
            delete [] m_modePredYuv[i];
            m_modePredYuv[i] = NULL;
        }
    }
    if (m_tmpResiYuv)
    {
        delete [] m_tmpResiYuv;
        m_tmpResiYuv = NULL;
    }
    if (m_tmpRecoYuv)
    {
        delete [] m_tmpRecoYuv;
        m_tmpRecoYuv = NULL;
    }
    if (m_origYuv)
    {
        delete [] m_origYuv;
        m_origYuv = NULL;
    }
}

/** \param    pcEncTop      pointer of encoder class
 */
Void TEncCu::init(TEncTop* top)
{
    m_cfg         = top;
    m_rateControl = top->getRateCtrl();
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  rpcCU pointer of CU data class
 */
#if CU_STAT_LOGFILE
extern int totalCU;
extern int cntInter[4], cntIntra[4], cntSplit[4], cntIntraNxN;
extern  int cuInterDistribution[4][4], cuIntraDistribution[4][3];
extern  int cntSkipCu[4],  cntTotalCu[4];
extern FILE* fp1;
bool mergeFlag = 0;
#endif

Void TEncCu::compressCU(TComDataCU* pcCu)
{
    // initialize CU data
    m_bestCU[0]->initCU(pcCu->getPic(), pcCu->getAddr());
    m_tempCU[0]->initCU(pcCu->getPic(), pcCu->getAddr());
#if CU_STAT_LOGFILE
    totalCU++;
    if (pcCu->getSlice()->getSliceType() != I_SLICE)
        fprintf(fp1, "CU number : %d \n", totalCU);
#endif
    //printf("compressCU[%2d]: Best=0x%08X, Temp=0x%08X\n", omp_get_thread_num(), m_ppcBestCU[0], m_ppcTempCU[0]);

    m_addSADDepth      = 0;
    m_LCUPredictionSAD = 0;
    m_temporalSAD      = 0;

    // analysis of CU

    if (m_bestCU[0]->getSlice()->getSliceType() == I_SLICE)
        xCompressIntraCU(m_bestCU[0], m_tempCU[0], NULL, 0);
    else
    {
        if (!m_cfg->getUseRDO())
        {
            TComDataCU* outBestCU = NULL;

            /* At the start of analysis, the best CU is a null pointer
            On return, it points to the CU encode with best chosen mode*/
            xCompressInterCU(outBestCU, m_tempCU[0], pcCu, 0, 0);
        }
        else
            xCompressCU(m_bestCU[0], m_tempCU[0], pcCu, 0, 0);
    }
    if (m_cfg->getUseAdaptQpSelect())
    {
        if (pcCu->getSlice()->getSliceType() != I_SLICE) //IIII
        {
            xLcuCollectARLStats(pcCu);
        }
    }
}

/** \param  cu  pointer of CU data class
 */
Void TEncCu::encodeCU(TComDataCU* cu)
{
    if (cu->getSlice()->getPPS()->getUseDQP())
    {
        setdQPFlag(true);
    }

    // Encode CU data
    xEncodeCU(cu, 0, 0);
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
 *\returns Void
*/
Void TEncCu::deriveTestModeAMP(TComDataCU *&outBestCU, PartSize parentSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver)
{
    if (outBestCU->getPartitionSize(0) == SIZE_2NxN)
    {
        bTestAMP_Hor = true;
    }
    else if (outBestCU->getPartitionSize(0) == SIZE_Nx2N)
    {
        bTestAMP_Ver = true;
    }
    else if (outBestCU->getPartitionSize(0) == SIZE_2Nx2N && outBestCU->getMergeFlag(0) == false && outBestCU->isSkipped(0) == false)
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

    if (outBestCU->getWidth(0) == 64)
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
 *\returns Void
 *
 *- for loop of QP value to compress the current CU with all possible QP
*/

Void TEncCu::xCompressIntraCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU* rpcParentBestCU, UInt depth, PartSize parentSize)
{
#if CU_STAT_LOGFILE
    cntTotalCu[depth]++;
    int boundaryCu = 0;
#endif
    m_abortFlag = false;
    TComPic* pcPic = outBestCU->getPic();

    //PPAScopeEvent(TEncCu_xCompressIntraCU + depth);

    // get Original YUV data from picture
    m_origYuv[depth]->copyFromPicYuv(pcPic->getPicYuvOrg(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU());

    // variables for fast encoder decision
    Bool    bTrySplit    = true;

    // variable for Early CU determination
    Bool    bSubBranch = true;

    // variable for Cbf fast mode PU decision
    Bool doNotBlockPu = true;
    Bool earlyDetectionSkipMode = false;

    Bool bTrySplitDQP  = true;

    Bool bBoundary = false;
    UInt uiLPelX   = outBestCU->getCUPelX();
    UInt uiRPelX   = uiLPelX + outBestCU->getWidth(0)  - 1;
    UInt uiTPelY   = outBestCU->getCUPelY();
    UInt uiBPelY   = uiTPelY + outBestCU->getHeight(0) - 1;

    Int iQP = m_cfg->getUseRateCtrl() ? m_rateControl->getRCQP() : outTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * pcSlice = outTempCU->getPic()->getSlice();
    Bool bSliceEnd = (pcSlice->getSliceCurEndCUAddr() > outTempCU->getSCUAddr() && pcSlice->getSliceCurEndCUAddr() < outTempCU->getSCUAddr() + outTempCU->getTotalNumPart());
    Bool bInsidePicture = (uiRPelX < outBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < outBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples());

    //Data for splitting
    UChar uhNextDepth = depth + 1;
    UInt partUnitIdx = 0;
    TComDataCU* pcSubBestPartCU[4], *pcSubTempPartCU[4];

    //We need to split; so dont try these modes
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit = true;

        outTempCU->initEstData(depth, iQP);

        bTrySplitDQP = bTrySplit;

        if (depth <= m_addSADDepth)
        {
            m_LCUPredictionSAD += m_temporalSAD;
            m_addSADDepth = depth;
        }

        outTempCU->initEstData(depth, iQP);

        xCheckRDCostIntra(outBestCU, outTempCU, SIZE_2Nx2N);
        outTempCU->initEstData(depth, iQP);

        if (depth == g_maxCUDepth - g_addCUDepth)
        {
            if (outTempCU->getWidth(0) > (1 << outTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()))
            {
                xCheckRDCostIntra(outBestCU, outTempCU, SIZE_NxN);
                outTempCU->initEstData(depth, iQP);
            }
        }

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth, true);
        outBestCU->getTotalBits() += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        outBestCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        outBestCU->getTotalCost()  = m_rdCost->calcRdCost(outBestCU->getTotalDistortion(), outBestCU->getTotalBits());

        // Early CU determination
        if (outBestCU->isSkipped(0))
        {
#if CU_STAT_LOGFILE
            cntSkipCu[depth]++;
#endif
            bSubBranch = false;
        }
        else
        {
            bSubBranch = true;
        }
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }

    outTempCU->initEstData(depth, iQP);

    // further split
    if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
    {
        for (; partUnitIdx < 4; partUnitIdx++)
        {
            pcSubBestPartCU[partUnitIdx]     = m_bestCU[uhNextDepth];
            pcSubTempPartCU[partUnitIdx]     = m_tempCU[uhNextDepth];

            pcSubBestPartCU[partUnitIdx]->initSubCU(outTempCU, partUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.
            pcSubTempPartCU[partUnitIdx]->initSubCU(outTempCU, partUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.

            Bool bInSlice = pcSubBestPartCU[partUnitIdx]->getSCUAddr() < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (pcSubBestPartCU[partUnitIdx]->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (pcSubBestPartCU[partUnitIdx]->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[uhNextDepth][CI_NEXT_BEST]);
                }

                // The following if condition has to be commented out in case the early Abort based on comparison of parentCu cost, childCU cost is not required.
                if (outBestCU->isIntra(0))
                {
                    xCompressIntraCU(pcSubBestPartCU[partUnitIdx], pcSubTempPartCU[partUnitIdx], outBestCU, uhNextDepth, SIZE_NONE);
                }
                else
                {
                    xCompressIntraCU(pcSubBestPartCU[partUnitIdx], pcSubTempPartCU[partUnitIdx], outBestCU, uhNextDepth, outBestCU->getPartitionSize(0));
                }
                {
                    outTempCU->copyPartFrom(pcSubBestPartCU[partUnitIdx], partUnitIdx, uhNextDepth); // Keep best part data to current temporary data.
                    xCopyYuv2Tmp(pcSubBestPartCU[partUnitIdx]->getTotalNumPart() * partUnitIdx, uhNextDepth);
                }
            }
            else if (bInSlice)
            {
                pcSubBestPartCU[partUnitIdx]->copyToPic(uhNextDepth);
                outTempCU->copyPartFrom(pcSubBestPartCU[partUnitIdx], partUnitIdx, uhNextDepth);
#if CU_STAT_LOGFILE
                boundaryCu++;
#endif
            }
        }

        if (!bBoundary)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(outTempCU, 0, depth, true);

            outTempCU->getTotalBits() += m_entropyCoder->getNumberOfWrittenBits();     // split bits
            outTempCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        }
        outTempCU->getTotalCost() = m_rdCost->calcRdCost(outTempCU->getTotalDistortion(), outTempCU->getTotalBits());

        if ((g_maxCUWidth >> depth) == outTempCU->getSlice()->getPPS()->getMinCuDQPSize() && outTempCU->getSlice()->getPPS()->getUseDQP())
        {
            Bool hasResidual = false;
            for (UInt uiBlkIdx = 0; uiBlkIdx < outTempCU->getTotalNumPart(); uiBlkIdx++)
            {
                if (outTempCU->getCbf(uiBlkIdx, TEXT_LUMA) || outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_U) || outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_V))
                {
                    hasResidual = true;
                    break;
                }
            }

            UInt uiTargetPartIdx;
            uiTargetPartIdx = 0;
            if (hasResidual)
            {
                Bool foundNonZeroCbf = false;
                outTempCU->setQPSubCUs(outTempCU->getRefQP(uiTargetPartIdx), outTempCU, 0, depth, foundNonZeroCbf);
                assert(foundNonZeroCbf);
            }
            else
            {
                outTempCU->setQPSubParts(outTempCU->getRefQP(uiTargetPartIdx), 0, depth);     // set QP to default QP
            }
        }

        m_rdSbacCoders[uhNextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
#if CU_STAT_LOGFILE
        if (outBestCU->getTotalCost() < outTempCU->getTotalCost())
        {
            cntIntra[depth]++;
            cntIntra[depth + 1] = cntIntra[depth + 1] - 4 + boundaryCu;
        }
#endif
        xCheckBestMode(outBestCU, outTempCU, depth);                                 // RD compare current larger prediction
        // with sub partitioned prediction.
    }

#if CU_STAT_LOGFILE
    if (depth == 3 && bSubBranch)
    {
        cntIntra[depth]++;
    }
#endif
    outBestCU->copyToPic(depth);                                                   // Copy Best data to Picture for next partition prediction.
    xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, uiLPelX, uiTPelY);   // Copy Yuv data to picture Yuv

    if (bBoundary || (bSliceEnd && bInsidePicture))
    {
        return;
    }

    // Assert if Best prediction mode is NONE
    // Selected mode's RD-cost must be not MAX_DOUBLE.
    assert(outBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(outBestCU->getPredictionMode(0) != MODE_NONE);
    assert(outBestCU->getTotalCost() != MAX_DOUBLE);
}

Void TEncCu::xCompressCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU* rpcParentBestCU, UInt depth, UInt /*partUnitIdx*/, PartSize parentSize)
{
#if CU_STAT_LOGFILE
    cntTotalCu[depth]++;
#endif
    m_abortFlag = false;
    TComPic* pcPic = outBestCU->getPic();

    //PPAScopeEvent(TEncCu_xCompressCU + depth);

    // get Original YUV data from picture
    m_origYuv[depth]->copyFromPicYuv(pcPic->getPicYuvOrg(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU());

    // variables for fast encoder decision
    Bool    bTrySplit    = true;

    // variable for Early CU determination
    Bool    bSubBranch = true;

    // variable for Cbf fast mode PU decision
    Bool doNotBlockPu = true;
    Bool earlyDetectionSkipMode = false;

    Bool bTrySplitDQP  = true;

    Bool bBoundary = false;
    UInt uiLPelX   = outBestCU->getCUPelX();
    UInt uiRPelX   = uiLPelX + outBestCU->getWidth(0)  - 1;
    UInt uiTPelY   = outBestCU->getCUPelY();
    UInt uiBPelY   = uiTPelY + outBestCU->getHeight(0) - 1;

    Int iQP = m_cfg->getUseRateCtrl() ? m_rateControl->getRCQP() : outTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * pcSlice = outTempCU->getPic()->getSlice();
    Bool bSliceEnd = (pcSlice->getSliceCurEndCUAddr() > outTempCU->getSCUAddr() && pcSlice->getSliceCurEndCUAddr() < outTempCU->getSCUAddr() + outTempCU->getTotalNumPart());
    Bool bInsidePicture = (uiRPelX < outBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < outBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples());
    // We need to split, so don't try these modes.
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit    = true;

        outTempCU->initEstData(depth, iQP);

        // do inter modes, SKIP and 2Nx2N
        if (outBestCU->getSlice()->getSliceType() != I_SLICE)
        {
            // 2Nx2N
            if (m_cfg->getUseEarlySkipDetection())
            {
                xCheckRDCostInter(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData(depth, iQP);                              //by Competition for inter_2Nx2N
            }
#if CU_STAT_LOGFILE
            mergeFlag = 1;
#endif
            // SKIP
            xCheckRDCostMerge2Nx2N(outBestCU, outTempCU, &earlyDetectionSkipMode, m_bestPredYuv[depth], m_bestRecoYuv[depth]); //by Merge for inter_2Nx2N
#if CU_STAT_LOGFILE
            mergeFlag = 0;
#endif
            outTempCU->initEstData(depth, iQP);

            if (!m_cfg->getUseEarlySkipDetection())
            {
                // 2Nx2N, NxN
                xCheckRDCostInter(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData(depth, iQP);
                if (m_cfg->getUseCbfFastMode())
                {
                    doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                }
            }
        }

        bTrySplitDQP = bTrySplit;

        if (depth <= m_addSADDepth)
        {
            m_LCUPredictionSAD += m_temporalSAD;
            m_addSADDepth = depth;
        }

        if (!earlyDetectionSkipMode)
        {
            outTempCU->initEstData(depth, iQP);

            // do inter modes, NxN, 2NxN, and Nx2N
            if (outBestCU->getSlice()->getSliceType() != I_SLICE)
            {
                // 2Nx2N, NxN
                if (!((outBestCU->getWidth(0) == 8) && (outBestCU->getHeight(0) == 8)))
                {
                    if (depth == g_maxCUDepth - g_addCUDepth && doNotBlockPu)
                    {
                        xCheckRDCostInter(outBestCU, outTempCU, SIZE_NxN);
                        outTempCU->initEstData(depth, iQP);
                    }
                }

                if (m_cfg->getUseRectInter())
                {
                    // 2NxN, Nx2N
                    if (doNotBlockPu)
                    {
                        xCheckRDCostInter(outBestCU, outTempCU, SIZE_Nx2N);
                        outTempCU->initEstData(depth, iQP);
                        if (m_cfg->getUseCbfFastMode() && outBestCU->getPartitionSize(0) == SIZE_Nx2N)
                        {
                            doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                    }
                    if (doNotBlockPu)
                    {
                        xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxN);
                        outTempCU->initEstData(depth, iQP);
                        if (m_cfg->getUseCbfFastMode() && outBestCU->getPartitionSize(0) == SIZE_2NxN)
                        {
                            doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                    }
                }

                //! Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
                if (pcPic->getSlice()->getSPS()->getAMPAcc(depth))
                {
                    Bool bTestAMP_Hor = false, bTestAMP_Ver = false;
                    Bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

                    deriveTestModeAMP(outBestCU, parentSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);

                    //! Do horizontal AMP
                    if (bTestAMP_Hor)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnU);
                            outTempCU->initEstData(depth, iQP);
                            if (m_cfg->getUseCbfFastMode() && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnD);
                            outTempCU->initEstData(depth, iQP);
                            if (m_cfg->getUseCbfFastMode() && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
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
                            outTempCU->initEstData(depth, iQP);
                            if (m_cfg->getUseCbfFastMode() && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_2NxnD, true);
                            outTempCU->initEstData(depth, iQP);
                            if (m_cfg->getUseCbfFastMode() && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                    }

                    //! Do horizontal AMP
                    if (bTestAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nLx2N);
                            outTempCU->initEstData(depth, iQP);
                            if (m_cfg->getUseCbfFastMode() && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nRx2N);
                            outTempCU->initEstData(depth, iQP);
                        }
                    }
                    else if (bTestMergeAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nLx2N, true);
                            outTempCU->initEstData(depth, iQP);
                            if (m_cfg->getUseCbfFastMode() && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
                            {
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(outBestCU, outTempCU, SIZE_nRx2N, true);
                            outTempCU->initEstData(depth, iQP);
                        }
                    }
                }
            }

            // do normal intra modes
            // speedup for inter frames
            if (outBestCU->getSlice()->getSliceType() == I_SLICE ||
                outBestCU->getCbf(0, TEXT_LUMA) != 0   ||
                outBestCU->getCbf(0, TEXT_CHROMA_U) != 0   ||
                outBestCU->getCbf(0, TEXT_CHROMA_V) != 0) // avoid very complex intra if it is unlikely
            {
                xCheckRDCostIntraInInter(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData(depth, iQP);

                if (depth == g_maxCUDepth - g_addCUDepth)
                {
                    if (outTempCU->getWidth(0) > (1 << outTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()))
                    {
                        xCheckRDCostIntraInInter(outBestCU, outTempCU, SIZE_NxN);
                        outTempCU->initEstData(depth, iQP);
                    }
                }
            }
            // test PCM
            if (pcPic->getSlice()->getSPS()->getUsePCM()
                && outTempCU->getWidth(0) <= (1 << pcPic->getSlice()->getSPS()->getPCMLog2MaxSize())
                && outTempCU->getWidth(0) >= (1 << pcPic->getSlice()->getSPS()->getPCMLog2MinSize()))
            {
                UInt uiRawBits = (2 * g_bitDepthY + g_bitDepthC) * outBestCU->getWidth(0) * outBestCU->getHeight(0) / 2;
                UInt uiBestBits = outBestCU->getTotalBits();
                if ((uiBestBits > uiRawBits) || (outBestCU->getTotalCost() > m_rdCost->calcRdCost(0, uiRawBits)))
                {
                    xCheckIntraPCM(outBestCU, outTempCU);
                    outTempCU->initEstData(depth, iQP);
                }
            }
        }

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth, true);
        outBestCU->getTotalBits() += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        outBestCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        outBestCU->getTotalCost()  = m_rdCost->calcRdCost(outBestCU->getTotalDistortion(), outBestCU->getTotalBits());

        // Early CU determination
        if (outBestCU->isSkipped(0))
        {
#if CU_STAT_LOGFILE
            cntSkipCu[depth]++;
#endif
            bSubBranch = false;
        }
        else
        {
            bSubBranch = true;
        }
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }

    // copy original YUV samples to PCM buffer
    if (outBestCU->isLosslessCoded(0) && (outBestCU->getIPCMFlag(0) == false))
    {
        xFillPCMBuffer(outBestCU, m_origYuv[depth]);
    }

    outTempCU->initEstData(depth, iQP);

    // further split
    if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
    {
        UChar       uhNextDepth         = depth + 1;
        TComDataCU* pcSubBestPartCU     = m_bestCU[uhNextDepth];
        TComDataCU* pcSubTempPartCU     = m_tempCU[uhNextDepth];
        UInt partUnitIdx = 0;
        for (; partUnitIdx < 4; partUnitIdx++)
        {
            pcSubBestPartCU->initSubCU(outTempCU, partUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.
            pcSubTempPartCU->initSubCU(outTempCU, partUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.

            Bool bInSlice = pcSubBestPartCU->getSCUAddr() < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (pcSubBestPartCU->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (pcSubBestPartCU->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[uhNextDepth][CI_NEXT_BEST]);
                }

                // The following if condition has to be commented out in case the early Abort based on comparison of parentCu cost, childCU cost is not required.
                if (outBestCU->isIntra(0))
                {
                    xCompressCU(pcSubBestPartCU, pcSubTempPartCU, outBestCU, uhNextDepth, SIZE_NONE);
                }
                else
                {
                    xCompressCU(pcSubBestPartCU, pcSubTempPartCU, outBestCU, uhNextDepth, outBestCU->getPartitionSize(0));
                }
                {
                    outTempCU->copyPartFrom(pcSubBestPartCU, partUnitIdx, uhNextDepth); // Keep best part data to current temporary data.
                    xCopyYuv2Tmp(pcSubBestPartCU->getTotalNumPart() * partUnitIdx, uhNextDepth);
                }
            }
            else if (bInSlice)
            {
                pcSubBestPartCU->copyToPic(uhNextDepth);
                outTempCU->copyPartFrom(pcSubBestPartCU, partUnitIdx, uhNextDepth);
            }
        }

        if (!bBoundary)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(outTempCU, 0, depth, true);

            outTempCU->getTotalBits() += m_entropyCoder->getNumberOfWrittenBits();     // split bits
            outTempCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        }
        outTempCU->getTotalCost() = m_rdCost->calcRdCost(outTempCU->getTotalDistortion(), outTempCU->getTotalBits());

        if ((g_maxCUWidth >> depth) == outTempCU->getSlice()->getPPS()->getMinCuDQPSize() && outTempCU->getSlice()->getPPS()->getUseDQP())
        {
            Bool hasResidual = false;
            for (UInt uiBlkIdx = 0; uiBlkIdx < outTempCU->getTotalNumPart(); uiBlkIdx++)
            {
                if (outTempCU->getCbf(uiBlkIdx, TEXT_LUMA) || outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_U) || outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_V))
                {
                    hasResidual = true;
                    break;
                }
            }

            UInt uiTargetPartIdx;
            uiTargetPartIdx = 0;
            if (hasResidual)
            {
                Bool foundNonZeroCbf = false;
                outTempCU->setQPSubCUs(outTempCU->getRefQP(uiTargetPartIdx), outTempCU, 0, depth, foundNonZeroCbf);
                assert(foundNonZeroCbf);
            }
            else
            {
                outTempCU->setQPSubParts(outTempCU->getRefQP(uiTargetPartIdx), 0, depth);     // set QP to default QP
            }
        }

        m_rdSbacCoders[uhNextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
#if  CU_STAT_LOGFILE
        if (outBestCU->getTotalCost() < outTempCU->getTotalCost())
        {
            if (outBestCU->getPredictionMode(0) == MODE_INTER)
            {
                cntInter[depth]++;
                if (outBestCU->getPartitionSize(0) < 3)
                {
                    cuInterDistribution[depth][outBestCU->getPartitionSize(0)]++;
                }
                else
                {
                    cuInterDistribution[depth][3]++;
                }
            }
            else if (outBestCU->getPredictionMode(0) == MODE_INTRA)
            {
                cntIntra[depth]++;
                if (outBestCU->getLumaIntraDir()[0] > 1)
                {
                    cuIntraDistribution[depth][2]++;
                }
                else
                {
                    cuIntraDistribution[depth][outBestCU->getLumaIntraDir()[0]]++;
                }
            }
        }
        else
        {
            cntSplit[depth]++;
        }
#endif // if  LOGGING
        xCheckBestMode(outBestCU, outTempCU, depth);                                 // RD compare current larger prediction
        // with sub partitioned prediction.
    }
#if CU_STAT_LOGFILE
    if (depth == 3)
    {
        if (!outBestCU->isSkipped(0))
        {
            if (outBestCU->getPredictionMode(0) == MODE_INTER)
            {
                cntInter[depth]++;
                if (outBestCU->getPartitionSize(0) < 3)
                {
                    cuInterDistribution[depth][outBestCU->getPartitionSize(0)]++;
                }
                else
                {
                    cuInterDistribution[depth][3]++;
                }
            }
            else if (outBestCU->getPredictionMode(0) == MODE_INTRA)
            {
                cntIntra[depth]++;
                if (outBestCU->getPartitionSize(0) == SIZE_2Nx2N)
                {
                    if (outBestCU->getLumaIntraDir()[0] > 1)
                    {
                        cuIntraDistribution[depth][2]++;
                    }
                    else
                    {
                        cuIntraDistribution[depth][outBestCU->getLumaIntraDir()[0]]++;
                    }
                }
                else if (outBestCU->getPartitionSize(0) == SIZE_NxN)
                {
                    cntIntraNxN++;
                }
            }
        }
    }
#endif // if LOGGING
    outBestCU->copyToPic(depth);                                                   // Copy Best data to Picture for next partition prediction.
    xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, uiLPelX, uiTPelY);   // Copy Yuv data to picture Yuv

    if (bBoundary || (bSliceEnd && bInsidePicture))
    {
        return;
    }

    // Assert if Best prediction mode is NONE
    // Selected mode's RD-cost must be not MAX_DOUBLE.
    assert(outBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(outBestCU->getPredictionMode(0) != MODE_NONE);
    assert(outBestCU->getTotalCost() != MAX_DOUBLE);
}

/** finish encoding a cu and handle end-of-slice conditions
 * \param cu
 * \param absPartIdx
 * \param depth
 * \returns Void
 */
Void TEncCu::finishCU(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    TComPic* pcPic = cu->getPic();
    TComSlice * pcSlice = cu->getPic()->getSlice();

    //Calculate end address
    UInt cuAddr = cu->getSCUAddr() + absPartIdx;

    UInt uiInternalAddress = (pcSlice->getSliceCurEndCUAddr() - 1) % pcPic->getNumPartInCU();
    UInt uiExternalAddress = (pcSlice->getSliceCurEndCUAddr() - 1) / pcPic->getNumPartInCU();
    UInt posx = (uiExternalAddress % pcPic->getFrameWidthInCU()) * g_maxCUWidth + g_rasterToPelX[g_zscanToRaster[uiInternalAddress]];
    UInt posy = (uiExternalAddress / pcPic->getFrameWidthInCU()) * g_maxCUHeight + g_rasterToPelY[g_zscanToRaster[uiInternalAddress]];
    UInt width = pcSlice->getSPS()->getPicWidthInLumaSamples();
    UInt height = pcSlice->getSPS()->getPicHeightInLumaSamples();

    while (posx >= width || posy >= height)
    {
        uiInternalAddress--;
        posx = (uiExternalAddress % pcPic->getFrameWidthInCU()) * g_maxCUWidth + g_rasterToPelX[g_zscanToRaster[uiInternalAddress]];
        posy = (uiExternalAddress / pcPic->getFrameWidthInCU()) * g_maxCUHeight + g_rasterToPelY[g_zscanToRaster[uiInternalAddress]];
    }

    uiInternalAddress++;
    if (uiInternalAddress == cu->getPic()->getNumPartInCU())
    {
        uiInternalAddress = 0;
        uiExternalAddress = (uiExternalAddress + 1);
    }
    UInt uiRealEndAddress = (uiExternalAddress * pcPic->getNumPartInCU() + uiInternalAddress);

    // Encode slice finish
    Bool bTerminateSlice = false;
    if (cuAddr + (cu->getPic()->getNumPartInCU() >> (depth << 1)) == uiRealEndAddress)
    {
        bTerminateSlice = true;
    }
    UInt uiGranularityWidth = g_maxCUWidth;
    posx = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    posy = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
    Bool granularityBoundary = ((posx + cu->getWidth(absPartIdx)) % uiGranularityWidth == 0 || (posx + cu->getWidth(absPartIdx) == width))
        && ((posy + cu->getHeight(absPartIdx)) % uiGranularityWidth == 0 || (posy + cu->getHeight(absPartIdx) == height));

    if (granularityBoundary)
    {
        // The 1-terminating bit is added to all streams, so don't add it here when it's 1.
        if (!bTerminateSlice)
            m_entropyCoder->encodeTerminatingBit(bTerminateSlice ? 1 : 0);
    }

    Int numberOfWrittenBits = 0;
    if (m_bitCounter)
    {
        numberOfWrittenBits = m_entropyCoder->getNumberOfWrittenBits();
    }

    // Calculate slice end IF this CU puts us over slice bit size.
    UInt iGranularitySize = cu->getPic()->getNumPartInCU();
    Int iGranularityEnd = ((cu->getSCUAddr() + absPartIdx) / iGranularitySize) * iGranularitySize;
    if (iGranularityEnd <= 0)
    {
        iGranularityEnd += max(iGranularitySize, (cu->getPic()->getNumPartInCU() >> (depth << 1)));
    }
    if (granularityBoundary)
    {
        pcSlice->setSliceBits((UInt)(pcSlice->getSliceBits() + numberOfWrittenBits));
        pcSlice->setSliceSegmentBits(pcSlice->getSliceSegmentBits() + numberOfWrittenBits);
        if (m_bitCounter)
        {
            m_entropyCoder->resetBits();
        }
    }
}

/** Compute QP for each CU
 * \param cu Target CU
 * \param depth CU depth
 * \returns quantization parameter
 */
Int TEncCu::xComputeQP(TComDataCU* cu, UInt depth)
{
    Int iBaseQp = cu->getSlice()->getSliceQp();
    Int iQpOffset = 0;

    if (m_cfg->getUseAdaptiveQP())
    {
        TEncPic* pcEPic = dynamic_cast<TEncPic*>(cu->getPic());
        UInt uiAQDepth = min(depth, pcEPic->getMaxAQDepth() - 1);
        TEncPicQPAdaptationLayer* pcAQLayer = pcEPic->getAQLayer(uiAQDepth);
        UInt uiAQUPosX = cu->getCUPelX() / pcAQLayer->getAQPartWidth();
        UInt uiAQUPosY = cu->getCUPelY() / pcAQLayer->getAQPartHeight();
        UInt uiAQUStride = pcAQLayer->getAQPartStride();
        TEncQPAdaptationUnit* acAQU = pcAQLayer->getQPAdaptationUnit();

        Double dMaxQScale = pow(2.0, m_cfg->getQPAdaptationRange() / 6.0);
        Double dAvgAct = pcAQLayer->getAvgActivity();
        Double dCUAct = acAQU[uiAQUPosY * uiAQUStride + uiAQUPosX].getActivity();
        Double dNormAct = (dMaxQScale * dCUAct + dAvgAct) / (dCUAct + dMaxQScale * dAvgAct);
        Double dQpOffset = log(dNormAct) / log(2.0) * 6.0;
        iQpOffset = Int(floor(dQpOffset + 0.49999));
    }
    return Clip3(-cu->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, iBaseQp + iQpOffset);
}

/** encode a CU block recursively
 * \param cu
 * \param absPartIdx
 * \param depth
 * \returns Void
 */
Void TEncCu::xEncodeCU(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    TComPic* pcPic = cu->getPic();

    Bool bBoundary = false;
    UInt uiLPelX   = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    UInt uiRPelX   = uiLPelX + (g_maxCUWidth >> depth)  - 1;
    UInt uiTPelY   = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
    UInt uiBPelY   = uiTPelY + (g_maxCUHeight >> depth) - 1;

    TComSlice * pcSlice = cu->getPic()->getSlice();

    // If slice start is within this cu...

    // We need to split, so don't try these modes.
    if ((uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
    {
        m_entropyCoder->encodeSplitFlag(cu, absPartIdx, depth);
    }
    else
    {
        bBoundary = true;
    }

    if (((depth < cu->getDepth(absPartIdx)) && (depth < (g_maxCUDepth - g_addCUDepth))) || bBoundary)
    {
        UInt uiQNumParts = (pcPic->getNumPartInCU() >> (depth << 1)) >> 2;
        if ((g_maxCUWidth >> depth) == cu->getSlice()->getPPS()->getMinCuDQPSize() && cu->getSlice()->getPPS()->getUseDQP())
        {
            setdQPFlag(true);
        }
        for (UInt partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++, absPartIdx += uiQNumParts)
        {
            uiLPelX   = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
            uiTPelY   = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
            Bool bInSlice = cu->getSCUAddr() + absPartIdx < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (uiLPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                xEncodeCU(cu, absPartIdx, depth + 1);
            }
        }

        return;
    }

    if ((g_maxCUWidth >> depth) >= cu->getSlice()->getPPS()->getMinCuDQPSize() && cu->getSlice()->getPPS()->getUseDQP())
    {
        setdQPFlag(true);
    }
    if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(cu, absPartIdx);
    }
    if (!cu->getSlice()->isIntra())
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
    Bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(cu, absPartIdx, depth, cu->getWidth(absPartIdx), cu->getHeight(absPartIdx), bCodeDQP);
    setdQPFlag(bCodeDQP);

    // --- write terminating bit ---
    finishCU(cu, absPartIdx, depth);
}

/** check RD costs for a CU block encoded with merge
 * \param outBestCU
 * \param outTempCU
 * \returns Void
 */
Void TEncCu::xCheckRDCostMerge2Nx2N(TComDataCU*& outBestCU, TComDataCU*& outTempCU, Bool *earlyDetectionSkipMode, TComYuv*& outBestPredYuv, TComYuv*& rpcYuvReconBest)
{
    assert(outTempCU->getSlice()->getSliceType() != I_SLICE);
    TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
    Int numValidMergeCand = 0;

    for (UInt ui = 0; ui < outTempCU->getSlice()->getMaxNumMergeCand(); ++ui)
    {
        uhInterDirNeighbours[ui] = 0;
    }

    UChar uhDepth = outTempCU->getDepth(0);
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uhDepth); // interprets depth relative to LCU level
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, uhDepth);
    outTempCU->getInterMergeCandidates(0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);

    Int mergeCandBuffer[MRG_MAX_NUM_CANDS];
    for (UInt ui = 0; ui < numValidMergeCand; ++ui)
    {
        mergeCandBuffer[ui] = 0;
    }

    Bool bestIsSkip = false;

    UInt iteration;
    if (outTempCU->isLosslessCoded(0))
    {
        iteration = 1;
    }
    else
    {
        iteration = 2;
    }

    for (UInt uiNoResidual = 0; uiNoResidual < iteration; ++uiNoResidual)
    {
        for (UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand)
        {
            if (!(uiNoResidual == 1 && mergeCandBuffer[uiMergeCand] == 1))
            {
                if (!(bestIsSkip && uiNoResidual == 0))
                {
                    // set MC parameters
                    outTempCU->setPredModeSubParts(MODE_INTER, 0, uhDepth); // interprets depth relative to LCU level
                    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(),     0, uhDepth);
                    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uhDepth); // interprets depth relative to LCU level
                    outTempCU->setMergeFlagSubParts(true, 0, 0, uhDepth); // interprets depth relative to LCU level
                    outTempCU->setMergeIndexSubParts(uiMergeCand, 0, 0, uhDepth); // interprets depth relative to LCU level
                    outTempCU->setInterDirSubParts(uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth); // interprets depth relative to LCU level
                    outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMvFieldNeighbours[0 + 2 * uiMergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level
                    outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMvFieldNeighbours[1 + 2 * uiMergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level

                    // do MC
                    m_search->motionCompensation(outTempCU, m_tmpPredYuv[uhDepth]);
                    // estimate residual and encode everything
                    m_search->encodeResAndCalcRdInterCU(outTempCU,
                                                              m_origYuv[uhDepth],
                                                              m_tmpPredYuv[uhDepth],
                                                              m_tmpResiYuv[uhDepth],
                                                              m_bestResiYuv[uhDepth],
                                                              m_tmpRecoYuv[uhDepth],
                                                              (uiNoResidual ? true : false));
                    /*Todo: Fix the satd cost estimates. Why is merge being chosen in high motion areas: estimated distortion is too low?*/
                    if (uiNoResidual == 0)
                    {
                        if (outTempCU->getQtRootCbf(0) == 0)
                        {
                            mergeCandBuffer[uiMergeCand] = 1;
                        }
                    }

                    outTempCU->setSkipFlagSubParts(outTempCU->getQtRootCbf(0) == 0, 0, uhDepth);
                    Int orgQP = outTempCU->getQP(0);
                    xCheckDQP(outTempCU);
                    if (outTempCU->getTotalCost() < outBestCU->getTotalCost())
                    {
                        TComDataCU* tmp = outTempCU;
                        outTempCU = outBestCU;
                        outBestCU = tmp;
                        // Change Prediction data
                        TComYuv* pcYuv = NULL;
                        pcYuv = outBestPredYuv;
                        outBestPredYuv  = m_tmpPredYuv[uhDepth];
                        m_tmpPredYuv[uhDepth] = pcYuv;
                        
                        pcYuv = rpcYuvReconBest;
                        rpcYuvReconBest = m_tmpRecoYuv[uhDepth];
                        m_tmpRecoYuv[uhDepth] = pcYuv;
                    }
                    outTempCU->initEstData(uhDepth, orgQP);
                    if (!bestIsSkip)
                    {
                        bestIsSkip = outBestCU->getQtRootCbf(0) == 0;
                    }
                }
            }
        }

        if (uiNoResidual == 0 && m_cfg->getUseEarlySkipDetection())
        {
            if (outBestCU->getQtRootCbf(0) == 0)
            {
                if (outBestCU->getMergeFlag(0))
                {
                    *earlyDetectionSkipMode = true;
                }
                else
                {
                    Int absoulte_MV = 0;
                    for (UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++)
                    {
                        if (outBestCU->getSlice()->getNumRefIdx(RefPicList(uiRefListIdx)) > 0)
                        {
                            TComCUMvField* pcCUMvField = outBestCU->getCUMvField(RefPicList(uiRefListIdx));
                            Int iHor = abs(pcCUMvField->getMvd(0).x);
                            Int iVer = abs(pcCUMvField->getMvd(0).y);
                            absoulte_MV += iHor + iVer;
                        }
                    }

                    if (absoulte_MV == 0)
                    {
                        *earlyDetectionSkipMode = true;
                    }
                }
            }
        }
    }
}

Void TEncCu::xCheckRDCostInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize, Bool bUseMRG)
{
    UChar uhDepth = outTempCU->getDepth(0);

    outTempCU->setDepthSubParts(uhDepth, 0);

    outTempCU->setSkipFlagSubParts(false, 0, uhDepth);

    outTempCU->setPartSizeSubParts(partSize,  0, uhDepth);
    outTempCU->setPredModeSubParts(MODE_INTER, 0, uhDepth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(),      0, uhDepth);

    outTempCU->setMergeAMP(true);
    m_tmpRecoYuv[uhDepth]->clear();
    m_tmpResiYuv[uhDepth]->clear();
    m_search->predInterSearch(outTempCU, m_origYuv[uhDepth], m_tmpPredYuv[uhDepth], bUseMRG);

    if (m_cfg->getUseRateCtrl() && m_cfg->getLCULevelRC() && partSize == SIZE_2Nx2N && uhDepth <= m_addSADDepth)
    {
        /* TODO: this needs to be tested with RC enabled, currently RC enabled x265 is not working */
        UInt partEnum = PartitionFromSizes(outTempCU->getWidth(0), outTempCU->getHeight(0));
        UInt SAD = primitives.sad[partEnum]((pixel*)m_origYuv[uhDepth]->getLumaAddr(), m_origYuv[uhDepth]->getStride(),
                                            (pixel*)m_tmpPredYuv[uhDepth]->getLumaAddr(), m_tmpPredYuv[uhDepth]->getStride());
        m_temporalSAD = (Int)SAD;
        x265_emms();
    }

    m_search->encodeResAndCalcRdInterCU(outTempCU, m_origYuv[uhDepth], m_tmpPredYuv[uhDepth], m_tmpResiYuv[uhDepth], m_bestResiYuv[uhDepth], m_tmpRecoYuv[uhDepth], false);

    xCheckDQP(outTempCU);

    xCheckBestMode(outBestCU, outTempCU, uhDepth);
}

Void TEncCu::xCheckRDCostIntra(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize)
{
    //PPAScopeEvent(TEncCU_xCheckRDCostIntra + depth);
    UInt depth = outTempCU->getDepth(0);
    UInt uiPreCalcDistC = 0;

    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    m_search->estIntraPredQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], uiPreCalcDistC, true);

    m_tmpRecoYuv[depth]->copyToPicLuma(outTempCU->getPic()->getPicYuvRec(), outTempCU->getAddr(), outTempCU->getZorderIdxInCU());

    m_search->estIntraPredChromaQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], uiPreCalcDistC);

    m_entropyCoder->resetBits();
    if (outTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(outTempCU, 0, true);
    }
    m_entropyCoder->encodeSkipFlag(outTempCU, 0,          true);
    m_entropyCoder->encodePredMode(outTempCU, 0,          true);
    m_entropyCoder->encodePartSize(outTempCU, 0, depth, true);
    m_entropyCoder->encodePredInfo(outTempCU, 0,          true);
    m_entropyCoder->encodeIPCMInfo(outTempCU, 0, true);

    // Encode Coefficients
    Bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(outTempCU, 0, depth, outTempCU->getWidth(0), outTempCU->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    outTempCU->getTotalBits() = m_entropyCoder->getNumberOfWrittenBits();
    outTempCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    outTempCU->getTotalCost() = m_rdCost->calcRdCost(outTempCU->getTotalDistortion(), outTempCU->getTotalBits());

    xCheckDQP(outTempCU);
    xCheckBestMode(outBestCU, outTempCU, depth);
}

Void TEncCu::xCheckRDCostIntraInInter(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize)
{
    UInt depth = outTempCU->getDepth(0);

    PPAScopeEvent(TEncCU_xCheckRDCostIntra + depth);

    outTempCU->setSkipFlagSubParts(false, 0, depth);

    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    Bool bSeparateLumaChroma = true; // choose estimation mode
    UInt uiPreCalcDistC      = 0;
    if (!bSeparateLumaChroma)
    {
        m_search->preestChromaPredMode(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth]);
    }
    m_search->estIntraPredQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], uiPreCalcDistC, bSeparateLumaChroma);

    m_tmpRecoYuv[depth]->copyToPicLuma(outTempCU->getPic()->getPicYuvRec(), outTempCU->getAddr(), outTempCU->getZorderIdxInCU());

    m_search->estIntraPredChromaQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], uiPreCalcDistC);

    m_entropyCoder->resetBits();
    if (outTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(outTempCU, 0, true);
    }
    m_entropyCoder->encodeSkipFlag(outTempCU, 0,          true);
    m_entropyCoder->encodePredMode(outTempCU, 0,          true);
    m_entropyCoder->encodePartSize(outTempCU, 0, depth, true);
    m_entropyCoder->encodePredInfo(outTempCU, 0,          true);
    m_entropyCoder->encodeIPCMInfo(outTempCU, 0, true);

    // Encode Coefficients
    Bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(outTempCU, 0, depth, outTempCU->getWidth(0), outTempCU->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    outTempCU->getTotalBits() = m_entropyCoder->getNumberOfWrittenBits();
    outTempCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    if (!m_cfg->getUseRDO())
    {
        UInt partEnum = PartitionFromSizes(outTempCU->getWidth(0), outTempCU->getHeight(0));
        UInt SATD = primitives.satd[partEnum]((pixel*)m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                              (pixel*)m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride());
        x265_emms();
        outTempCU->getTotalDistortion() = SATD;
    }
    outTempCU->getTotalCost() = m_rdCost->calcRdCost(outTempCU->getTotalDistortion(), outTempCU->getTotalBits());

    xCheckDQP(outTempCU);
    xCheckBestMode(outBestCU, outTempCU, depth);
}

/** Check R-D costs for a CU with PCM mode.
 * \param outBestCU pointer to best mode CU data structure
 * \param outTempCU pointer to testing mode CU data structure
 * \returns Void
 *
 * \note Current PCM implementation encodes sample values in a lossless way. The distortion of PCM mode CUs are zero. PCM mode is selected if the best mode yields bits greater than that of PCM mode.
 */
Void TEncCu::xCheckIntraPCM(TComDataCU*& outBestCU, TComDataCU*& outTempCU)
{
    UInt depth = outTempCU->getDepth(0);

    //PPAScopeEvent(TEncCU_xCheckIntraPCM);

    outTempCU->setSkipFlagSubParts(false, 0, depth);

    outTempCU->setIPCMFlag(0, true);
    outTempCU->setIPCMFlagSubParts(true, 0, outTempCU->getDepth(0));
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setTrIdxSubParts(0, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    m_search->IPCMSearch(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);

    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

    m_entropyCoder->resetBits();
    if (outTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(outTempCU, 0,          true);
    }
    m_entropyCoder->encodeSkipFlag(outTempCU, 0,          true);
    m_entropyCoder->encodePredMode(outTempCU, 0,          true);
    m_entropyCoder->encodePartSize(outTempCU, 0, depth, true);
    m_entropyCoder->encodeIPCMInfo(outTempCU, 0, true);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    outTempCU->getTotalBits() = m_entropyCoder->getNumberOfWrittenBits();
    outTempCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    outTempCU->getTotalCost() = m_rdCost->calcRdCost(outTempCU->getTotalDistortion(), outTempCU->getTotalBits());

    xCheckDQP(outTempCU);
    xCheckBestMode(outBestCU, outTempCU, depth);
}

/** check whether current try is the best with identifying the depth of current try
 * \param outBestCU
 * \param outTempCU
 * \returns Void
 */
Void TEncCu::xCheckBestMode(TComDataCU*& outBestCU, TComDataCU*& outTempCU, UInt depth)
{
    if (outTempCU->getTotalCost() < outBestCU->getTotalCost())
    {
        TComYuv* pcYuv;
        // Change Information data
        TComDataCU* cu = outBestCU;
        outBestCU = outTempCU;
        outTempCU = cu;

        // Change Prediction data
        pcYuv = m_bestPredYuv[depth];
        m_bestPredYuv[depth] = m_tmpPredYuv[depth];
        m_tmpPredYuv[depth] = pcYuv;

        // Change Reconstruction data
        pcYuv = m_bestRecoYuv[depth];
        m_bestRecoYuv[depth] = m_tmpRecoYuv[depth];
        m_tmpRecoYuv[depth] = pcYuv;

        pcYuv = NULL;
        cu  = NULL;

        m_rdSbacCoders[depth][CI_TEMP_BEST]->store(m_rdSbacCoders[depth][CI_NEXT_BEST]);
    }
}

Void TEncCu::xCheckDQP(TComDataCU* cu)
{
    UInt depth = cu->getDepth(0);

    if (cu->getSlice()->getPPS()->getUseDQP() && (g_maxCUWidth >> depth) >= cu->getSlice()->getPPS()->getMinCuDQPSize())
    {
        cu->setQPSubParts(cu->getRefQP(0), 0, depth); // set QP to default QP
    }
}

Void TEncCu::xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst)
{
    dst->iN = src->iN;
    for (Int i = 0; i < src->iN; i++)
    {
        dst->m_acMvCand[i] = src->m_acMvCand[i];
    }
}

Void TEncCu::xCopyYuv2Pic(TComPic* outPic, UInt cuAddr, UInt absPartIdx, UInt depth, UInt uiSrcDepth, TComDataCU* cu, UInt uiLPelX, UInt uiTPelY)
{
    UInt uiRPelX   = uiLPelX + (g_maxCUWidth >> depth)  - 1;
    UInt uiBPelY   = uiTPelY + (g_maxCUHeight >> depth) - 1;
    TComSlice * pcSlice = cu->getPic()->getSlice();
    Bool bSliceEnd   = pcSlice->getSliceCurEndCUAddr() > (cu->getAddr()) * cu->getPic()->getNumPartInCU() + absPartIdx &&
        pcSlice->getSliceCurEndCUAddr() < (cu->getAddr()) * cu->getPic()->getNumPartInCU() + absPartIdx + (cu->getPic()->getNumPartInCU() >> (depth << 1));

    if (!bSliceEnd && (uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
    {
        UInt uiAbsPartIdxInRaster = g_zscanToRaster[absPartIdx];
        UInt uiSrcBlkWidth = outPic->getNumPartInWidth() >> (uiSrcDepth);
        UInt uiBlkWidth    = outPic->getNumPartInWidth() >> (depth);
        UInt uiPartIdxX = ((uiAbsPartIdxInRaster % outPic->getNumPartInWidth()) % uiSrcBlkWidth) / uiBlkWidth;
        UInt uiPartIdxY = ((uiAbsPartIdxInRaster / outPic->getNumPartInWidth()) % uiSrcBlkWidth) / uiBlkWidth;
        UInt partIdx = uiPartIdxY * (uiSrcBlkWidth / uiBlkWidth) + uiPartIdxX;
        m_bestRecoYuv[uiSrcDepth]->copyToPicYuv(outPic->getPicYuvRec(), cuAddr, absPartIdx, depth - uiSrcDepth, partIdx);
    }
    else
    {
        UInt uiQNumParts = (cu->getPic()->getNumPartInCU() >> (depth << 1)) >> 2;

        for (UInt partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++, absPartIdx += uiQNumParts)
        {
            UInt uiSubCULPelX   = uiLPelX + (g_maxCUWidth >> (depth + 1)) * (partUnitIdx &  1);
            UInt uiSubCUTPelY   = uiTPelY + (g_maxCUHeight >> (depth + 1)) * (partUnitIdx >> 1);

            Bool bInSlice = cu->getAddr() * cu->getPic()->getNumPartInCU() + absPartIdx < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (uiSubCULPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiSubCUTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                xCopyYuv2Pic(outPic, cuAddr, absPartIdx, depth + 1, uiSrcDepth, cu, uiSubCULPelX, uiSubCUTPelY); // Copy Yuv data to picture Yuv
            }
        }
    }
}

Void TEncCu::xCopyYuv2Tmp(UInt partUnitIdx, UInt uiNextDepth)
{
    UInt uiCurrDepth = uiNextDepth - 1;

    m_bestRecoYuv[uiNextDepth]->copyToPartYuv(m_tmpRecoYuv[uiCurrDepth], partUnitIdx);
}

Void TEncCu::xCopyYuv2Best(UInt partUnitIdx, UInt uiNextDepth)
{
    UInt uiCurrDepth = uiNextDepth - 1;

    m_tmpRecoYuv[uiNextDepth]->copyToPartYuv(m_bestRecoYuv[uiCurrDepth], partUnitIdx);
}

/** Function for filling the PCM buffer of a CU using its original sample array
 * \param cu pointer to current CU
 * \param fencYuv pointer to original sample array
 * \returns Void
 */
Void TEncCu::xFillPCMBuffer(TComDataCU*& pCU, TComYuv* pOrgYuv)
{
    UInt   width        = pCU->getWidth(0);
    UInt   height       = pCU->getHeight(0);

    Pel*   pSrcY = pOrgYuv->getLumaAddr(0, width);
    Pel*   pDstY = pCU->getPCMSampleY();
    UInt   srcStride = pOrgYuv->getStride();

    for (Int y = 0; y < height; y++)
    {
        for (Int x = 0; x < width; x++)
        {
            pDstY[x] = pSrcY[x];
        }

        pDstY += width;
        pSrcY += srcStride;
    }

    Pel* pSrcCb       = pOrgYuv->getCbAddr();
    Pel* pSrcCr       = pOrgYuv->getCrAddr();

    Pel* pDstCb       = pCU->getPCMSampleCb();
    Pel* pDstCr       = pCU->getPCMSampleCr();

    UInt srcStrideC = pOrgYuv->getCStride();
    UInt heightC   = height >> 1;
    UInt widthC    = width  >> 1;

    for (Int y = 0; y < heightC; y++)
    {
        for (Int x = 0; x < widthC; x++)
        {
            pDstCb[x] = pSrcCb[x];
            pDstCr[x] = pSrcCr[x];
        }

        pDstCb += widthC;
        pDstCr += widthC;
        pSrcCb += srcStrideC;
        pSrcCr += srcStrideC;
    }
}

/** Collect ARL statistics from one block
  */
Int TEncCu::xTuCollectARLStats(TCoeff* rpcCoeff, Int* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples)
{
    for (Int n = 0; n < NumCoeffInCU; n++)
    {
        Int u = abs(rpcCoeff[n]);
        Int absc = rpcArlCoeff[n];

        if (u != 0)
        {
            if (u < LEVEL_RANGE)
            {
                cSum[u] += (Double)absc;
                numSamples[u]++;
            }
            else
            {
                cSum[LEVEL_RANGE] += (Double)absc - (Double)(u << ARL_C_PRECISION);
                numSamples[LEVEL_RANGE]++;
            }
        }
    }

    return 0;
}

/** Collect ARL statistics from one LCU
 * \param cu
 */
Void TEncCu::xLcuCollectARLStats(TComDataCU* rpcCU)
{
    Double cSum[LEVEL_RANGE + 1];     //: the sum of DCT coefficients corresponding to datatype and quantization output
    UInt numSamples[LEVEL_RANGE + 1]; //: the number of coefficients corresponding to datatype and quantization output

    TCoeff* pCoeffY = rpcCU->getCoeffY();
    Int* pArlCoeffY = rpcCU->getArlCoeffY();

    UInt uiMinCUWidth = g_maxCUWidth >> g_maxCUDepth;
    UInt uiMinNumCoeffInCU = 1 << uiMinCUWidth;

    memset(cSum, 0, sizeof(Double) * (LEVEL_RANGE + 1));
    memset(numSamples, 0, sizeof(UInt) * (LEVEL_RANGE + 1));

    // Collect stats to cSum[][] and numSamples[][]
    for (Int i = 0; i < rpcCU->getTotalNumPart(); i++)
    {
        UInt uiTrIdx = rpcCU->getTransformIdx(i);

        if (rpcCU->getPredictionMode(i) == MODE_INTER)
        {
            if (rpcCU->getCbf(i, TEXT_LUMA, uiTrIdx))
            {
                xTuCollectARLStats(pCoeffY, pArlCoeffY, uiMinNumCoeffInCU, cSum, numSamples);
            } //Note that only InterY is processed. QP rounding is based on InterY data only.
        }

        pCoeffY  += uiMinNumCoeffInCU;
        pArlCoeffY  += uiMinNumCoeffInCU;
    }

    for (Int u = 1; u < LEVEL_RANGE; u++)
    {
        m_trQuant->getSliceSumC()[u] += cSum[u];
        m_trQuant->getSliceNSamples()[u] += numSamples[u];
    }

    m_trQuant->getSliceSumC()[LEVEL_RANGE] += cSum[LEVEL_RANGE];
    m_trQuant->getSliceNSamples()[LEVEL_RANGE] += numSamples[LEVEL_RANGE];
}

//! \}
