/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "TLibEncoder/TEncCu.h"
#include <math.h>
#include <common.h>

using namespace x265;

#if CU_STAT_LOGFILE
extern int totalCU;
extern int cntInter[4], cntIntra[4], cntSplit[4], cntIntraNxN;
extern  int cuInterDistribution[4][4], cuIntraDistribution[4][3];
extern  int cntSkipCu[4],  cntTotalCu[4];
extern FILE* fp1;
#endif

Void TEncCu::xEncodeIntrainInter(TComDataCU*& pcCU, TComYuv* pcYuvOrg, TComYuv* pcYuvPred,  TShortYUV*& rpcYuvResi, TComYuv*& rpcYuvRec)
{
    UInt64   dPUCost = 0;
    UInt   uiPUDistY = 0;
    UInt   uiPUDistC = 0;
    UInt   uiDepth = pcCU->getDepth(0);
    UInt    uiInitTrDepth  = pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;

    // set context models
    m_pcPredSearch->m_pcRDGoOnSbacCoder->load(m_pcPredSearch->m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

    m_pcPredSearch->xRecurIntraCodingQT(pcCU, uiInitTrDepth, 0, true, pcYuvOrg, pcYuvPred, rpcYuvResi, uiPUDistY, uiPUDistC, false, dPUCost);
    m_pcPredSearch->xSetIntraResultQT(pcCU, uiInitTrDepth, 0, true, rpcYuvRec);

    //=== update PU data ====
    pcCU->copyToPic(pcCU->getDepth(0), 0, uiInitTrDepth);

    //===== set distortion (rate and r-d costs are determined later) =====
    pcCU->getTotalDistortion() = uiPUDistY + uiPUDistC;

    rpcYuvRec->copyToPicLuma(pcCU->getPic()->getPicYuvRec(), pcCU->getAddr(), pcCU->getZorderIdxInCU());

    m_pcPredSearch->estIntraPredChromaQT(pcCU, pcYuvOrg, pcYuvPred, rpcYuvResi, rpcYuvRec, uiPUDistC);
    m_pcEntropyCoder->resetBits();
    if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
    }
    m_pcEntropyCoder->encodeSkipFlag(pcCU, 0,          true);
    m_pcEntropyCoder->encodePredMode(pcCU, 0,          true);
    m_pcEntropyCoder->encodePartSize(pcCU, 0, uiDepth, true);
    m_pcEntropyCoder->encodePredInfo(pcCU, 0,          true);
    m_pcEntropyCoder->encodeIPCMInfo(pcCU, 0, true);

    // Encode Coefficients
    Bool bCodeDQP = getdQPFlag();
    m_pcEntropyCoder->encodeCoeff(pcCU, 0, uiDepth, pcCU->getWidth(0), pcCU->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

    pcCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
    pcCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    pcCU->getTotalCost() = m_pcRdCost->calcRdCost(pcCU->getTotalDistortion(), pcCU->getTotalBits());
}

Void TEncCu::xComputeCostIntrainInter(TComDataCU*& pcCU, PartSize eSize)
{
    UInt    uiDepth        = pcCU->getDepth(0);

    pcCU->setSkipFlagSubParts(false, 0, uiDepth);
    pcCU->setPartSizeSubParts(eSize, 0, uiDepth);
    pcCU->setPredModeSubParts(MODE_INTRA, 0, uiDepth);
    pcCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(), 0, uiDepth);

    UInt    uiInitTrDepth  = pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    UInt    uiWidth        = pcCU->getWidth(0) >> uiInitTrDepth;
    UInt    uiWidthBit     = pcCU->getIntraSizeIdx(0);
    UInt64  CandCostList[FAST_UDI_MAX_RDMODE_NUM];
    UInt    CandNum;

    UInt uiPartOffset = 0;

    //===== init pattern for luma prediction =====
    pcCU->getPattern()->initPattern(pcCU, uiInitTrDepth, uiPartOffset);
    // Reference sample smoothing
    pcCU->getPattern()->initAdiPattern(pcCU, uiPartOffset, uiInitTrDepth, m_pcPredSearch->getPredicBuf(),  m_pcPredSearch->getPredicBufWidth(),  m_pcPredSearch->getPredicBufHeight(), m_pcPredSearch->refAbove, m_pcPredSearch->refLeft, m_pcPredSearch->refAboveFlt, m_pcPredSearch->refLeftFlt);

    //===== determine set of modes to be tested (using prediction signal only) =====
    UInt numModesAvailable = 35; //total number of Intra modes
    Pel* piOrg         = m_ppcOrigYuv[uiDepth]->getLumaAddr(0, uiWidth);
    Pel* piPred        = m_ppcPredYuvMode[5][uiDepth]->getLumaAddr(0, uiWidth);
    UInt uiStride      = m_ppcPredYuvMode[5][uiDepth]->getStride();
    UInt uiRdModeList[FAST_UDI_MAX_RDMODE_NUM];
    UInt numModesForFullRD = g_intraModeNumFast[uiWidthBit];
    Int nLog2SizeMinus2 = g_convertToBit[uiWidth];
    x265::pixelcmp_t sa8d = x265::primitives.sa8d[nLog2SizeMinus2];
    {
        assert(numModesForFullRD < numModesAvailable);

        for (UInt i = 0; i < numModesForFullRD; i++)
        {
            CandCostList[i] = MAX_INT64;
        }

        CandNum = 0;
        UInt uiSads[35];
        Bool bFilter = (uiWidth <= 16);
        Pel *ptrSrc = m_pcPredSearch->getPredicBuf();

        // 1
        primitives.intra_pred_dc((pixel*)ptrSrc + ADI_BUF_STRIDE + 1, ADI_BUF_STRIDE, (pixel*)piPred, uiStride, uiWidth, bFilter);
        uiSads[DC_IDX] = sa8d((pixel*)piOrg, uiStride, (pixel*)piPred, uiStride);

        // 0
        if (uiWidth >= 8 && uiWidth <= 32)
        {
            ptrSrc += ADI_BUF_STRIDE * (2 * uiWidth + 1);
        }
        primitives.intra_pred_planar((pixel*)ptrSrc + ADI_BUF_STRIDE + 1, ADI_BUF_STRIDE, (pixel*)piPred, uiStride, uiWidth);
        uiSads[PLANAR_IDX] = sa8d((pixel*)piOrg, uiStride, (pixel*)piPred, uiStride);

        // 33 Angle modes once
        if (uiWidth <= 16)
        {
            ALIGN_VAR_32(Pel, buf1[MAX_CU_SIZE * MAX_CU_SIZE]);
            ALIGN_VAR_32(Pel, tmp[33 * MAX_CU_SIZE * MAX_CU_SIZE]);

            // Transpose NxN
            x265::primitives.transpose[nLog2SizeMinus2]((pixel*)buf1, (pixel*)piOrg, uiStride);

            Pel *pAbove0 = m_pcPredSearch->refAbove    + uiWidth - 1;
            Pel *pAbove1 = m_pcPredSearch->refAboveFlt + uiWidth - 1;
            Pel *pLeft0  = m_pcPredSearch->refLeft     + uiWidth - 1;
            Pel *pLeft1  = m_pcPredSearch->refLeftFlt  + uiWidth - 1;

            x265::primitives.intra_pred_allangs[nLog2SizeMinus2]((pixel*)tmp, (pixel*)pAbove0, (pixel*)pLeft0, (pixel*)pAbove1, (pixel*)pLeft1, (uiWidth <= 16));

            // TODO: We need SATD_x4 here
            for (UInt uiMode = 2; uiMode < numModesAvailable; uiMode++)
            {
                bool modeHor = (uiMode < 18);
                Pel *pSrc = (modeHor ? buf1 : piOrg);
                intptr_t srcStride = (modeHor ? uiWidth : uiStride);

                // use hadamard transform here
                UInt uiSad = sa8d((pixel*)pSrc, srcStride, (pixel*)&tmp[(uiMode - 2) * (uiWidth * uiWidth)], uiWidth);
                uiSads[uiMode] = uiSad;
            }
        }
        else
        {
            for (UInt uiMode = 2; uiMode < numModesAvailable; uiMode++)
            {
                m_pcPredSearch->predIntraLumaAng(pcCU->getPattern(), uiMode, piPred, uiStride, uiWidth);

                // use hadamard transform here
                UInt uiSad = sa8d((pixel*)piOrg, uiStride, (pixel*)piPred, uiStride);
                uiSads[uiMode] = uiSad;
            }
        }

        for (UInt uiMode = 0; uiMode < numModesAvailable; uiMode++)
        {
            UInt uiSad = uiSads[uiMode];
            UInt iModeBits = m_pcPredSearch->xModeBitsIntra(pcCU, uiMode, 0, uiPartOffset, uiDepth, uiInitTrDepth);
            UInt64 cost = m_pcRdCost->calcRdSADCost(uiSad, iModeBits);
            CandNum += m_pcPredSearch->xUpdateCandList(uiMode, cost, numModesForFullRD, uiRdModeList, CandCostList);      //Find N least cost  modes. N = numModesForFullRD
        }

        Int uiPreds[3] = { -1, -1, -1 };
        Int iMode = -1;
        Int numCand = pcCU->getIntraDirLumaPredictor(uiPartOffset, uiPreds, &iMode);
        if (iMode >= 0)
        {
            numCand = iMode;
        }

        for (Int j = 0; j < numCand; j++)
        {
            Bool mostProbableModeIncluded = false;
            UInt mostProbableMode = uiPreds[j];

            for (UInt i = 0; i < numModesForFullRD; i++)
            {
                mostProbableModeIncluded |= (mostProbableMode == uiRdModeList[i]);
            }

            if (!mostProbableModeIncluded)
            {
                uiRdModeList[numModesForFullRD++] = mostProbableMode;
            }
        }
    }

    //determine predyuv for the best mode
    UInt uiOrgMode = uiRdModeList[0];

    pcCU->setLumaIntraDirSubParts(uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth);

    // set context models
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);    
}

/** check RD costs for a CU block encoded with merge
 * \param rpcBestCU
 * \param rpcTempCU
 * \returns Void
 */

Void TEncCu::xComputeCostMerge2Nx2N(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool *earlyDetectionSkipMode)
{
    assert(rpcTempCU->getSlice()->getSliceType() != I_SLICE);
    TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
    Int numValidMergeCand = 0;

    for (UInt ui = 0; ui < rpcTempCU->getSlice()->getMaxNumMergeCand(); ++ui)
    {
        uhInterDirNeighbours[ui] = 0;
    }

    UChar uhDepth = rpcTempCU->getDepth(0);
    rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uhDepth); // interprets depth relative to LCU level
    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(), 0, uhDepth);
    rpcTempCU->getInterMergeCandidates(0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);

    Int mergeCandBuffer[MRG_MAX_NUM_CANDS];
    for (Int ui = 0; ui < numValidMergeCand; ++ui)
    {
        mergeCandBuffer[ui] = 0;
    }

    Bool bestIsSkip = false;

    UInt iteration;
    if (rpcTempCU->isLosslessCoded(0))
    {
        iteration = 1;
    }
    else
    {
        iteration = 2;
    }

    for (UInt uiNoResidual = 1; uiNoResidual < iteration; ++uiNoResidual)
    {
        for (Int uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand)
        {
            if (!(uiNoResidual == 1 && mergeCandBuffer[uiMergeCand] == 1))
            {
                if (!(bestIsSkip && uiNoResidual == 0))
                {
                    // set MC parameters
                    rpcTempCU->setPredModeSubParts(MODE_INTER, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(),     0, uhDepth);
                    rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->setMergeFlagSubParts(true, 0, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->setMergeIndexSubParts(uiMergeCand, 0, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->setInterDirSubParts(uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMvFieldNeighbours[0 + 2 * uiMergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level
                    rpcTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMvFieldNeighbours[1 + 2 * uiMergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level

                    // do MC
                    m_pcPredSearch->motionCompensation(rpcTempCU, m_ppcPredYuvMode[4][uhDepth]);
                    // estimate residual and encode everything
                    m_pcPredSearch->encodeResAndCalcRdInterCU(rpcTempCU,
                                                              m_ppcOrigYuv[uhDepth],
                                                              m_ppcPredYuvMode[4][uhDepth],
                                                              m_ppcResiYuvTemp[uhDepth],
                                                              m_ppcResiYuvBest[uhDepth],
                                                              m_ppcRecoYuvTemp[uhDepth],
                                                              (uiNoResidual ? true : false));
                    /*Todo: Fix the satd cost estimates. Why is merge being chosen in high motion areas: estimated distortion is too low?*/
                    if (uiNoResidual == 0)
                    {
                        if (rpcTempCU->getQtRootCbf(0) == 0)
                        {
                            mergeCandBuffer[uiMergeCand] = 1;
                        }
                    }

                    rpcTempCU->setSkipFlagSubParts(rpcTempCU->getQtRootCbf(0) == 0, 0, uhDepth);
                    Int orgQP = rpcTempCU->getQP(0);
                    xCheckDQP(rpcTempCU);
                    if (rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost())
                    {
                        TComDataCU* tmp = rpcTempCU;
                        rpcTempCU = rpcBestCU;
                        rpcBestCU = tmp;
                        // Change Prediction data
                        TComYuv* pcYuv = NULL;
                        pcYuv = m_ppcPredYuvMode[3][uhDepth];
                        m_ppcPredYuvMode[3][uhDepth]  = m_ppcPredYuvMode[4][uhDepth];
                        m_ppcPredYuvMode[4][uhDepth] = pcYuv;
                        pcYuv = m_ppcRecoYuvBest[uhDepth];
                        m_ppcRecoYuvBest[uhDepth] = m_ppcRecoYuvTemp[uhDepth];
                        m_ppcRecoYuvTemp[uhDepth] = pcYuv;
                    }
                    
                    rpcTempCU->initEstData(uhDepth, orgQP);

                    if (m_pcEncCfg->getUseFastDecisionForMerge() && !bestIsSkip)
                    {
                        bestIsSkip = rpcBestCU->getQtRootCbf(0) == 0;
                    }
                }
            }
        }

        if (uiNoResidual == 0 && m_pcEncCfg->getUseEarlySkipDetection())
        {
            if (rpcBestCU->getQtRootCbf(0) == 0)
            {
                if (rpcBestCU->getMergeFlag(0))
                {
                    *earlyDetectionSkipMode = true;
                }
                else
                {
                    Int absoulte_MV = 0;
                    for (UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++)
                    {
                        if (rpcBestCU->getSlice()->getNumRefIdx(RefPicList(uiRefListIdx)) > 0)
                        {
                            TComCUMvField* pcCUMvField = rpcBestCU->getCUMvField(RefPicList(uiRefListIdx));
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

Void TEncCu::xComputeCostInter(TComDataCU*& rpcTempCU, PartSize ePartSize, UInt Index, Bool bUseMRG)
{
    UChar uhDepth = rpcTempCU->getDepth(0);

    rpcTempCU->setDepthSubParts(uhDepth, 0);

    rpcTempCU->setSkipFlagSubParts(false, 0, uhDepth);

    rpcTempCU->setPartSizeSubParts(ePartSize,  0, uhDepth);
    rpcTempCU->setPredModeSubParts(MODE_INTER, 0, uhDepth);
    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(),      0, uhDepth);

    rpcTempCU->setMergeAMP(true);
    m_ppcRecoYuvTemp[uhDepth]->clear();
    m_ppcResiYuvTemp[uhDepth]->clear();

    m_pcPredSearch->predInterSearch(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvMode[Index][uhDepth], bUseMRG);
}

Void TEncCu::xCompressInterCU(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, TComDataCU*& pcCU, UInt uiDepth, UInt PartitionIndex)
{
#if CU_STAT_LOGFILE
    cntTotalCu[uiDepth]++;
#endif
    m_abortFlag = false;

    TComPic* pcPic = rpcTempCU->getPic();

    // get Original YUV data from picture
    m_ppcOrigYuv[uiDepth]->copyFromPicYuv(pcPic->getPicYuvOrg(), rpcTempCU->getAddr(), rpcTempCU->getZorderIdxInCU());

    // variables for fast encoder decision
    Bool    bTrySplit    = true;

    // variable for Early CU determination
    Bool    bSubBranch = true;

    Bool bTrySplitDQP  = true;

    Bool bBoundary = false;
    UInt uiLPelX   = rpcTempCU->getCUPelX();
    UInt uiRPelX   = uiLPelX + rpcTempCU->getWidth(0)  - 1;
    UInt uiTPelY   = rpcTempCU->getCUPelY();
    UInt uiBPelY   = uiTPelY + rpcTempCU->getHeight(0) - 1;

    Int iQP = m_pcEncCfg->getUseRateCtrl() ? m_pcRateCtrl->getRCQP() : rpcTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * pcSlice = rpcTempCU->getPic()->getSlice();
    Bool bSliceEnd = (pcSlice->getSliceCurEndCUAddr() > rpcTempCU->getSCUAddr() && pcSlice->getSliceCurEndCUAddr() < rpcTempCU->getSCUAddr() + rpcTempCU->getTotalNumPart());
    Bool bInsidePicture = (uiRPelX < rpcTempCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < rpcTempCU->getSlice()->getSPS()->getPicHeightInLumaSamples());
    // We need to split, so don't try these modes.
    TComYuv* YuvTemp = NULL;
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit    = true;

        /*Initialise all Mode-CUs based on parentCU*/
        if (uiDepth == 0)
        {
            m_InterCU_2Nx2N[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_InterCU_Nx2N[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_InterCU_2NxN[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_IntraInInterCU[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_MergeCU[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_MergeBestCU[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
        }
        else
        {
            m_InterCU_2Nx2N[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_InterCU_2NxN[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_InterCU_Nx2N[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_IntraInInterCU[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_MergeCU[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_MergeBestCU[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
        }

        /*Compute  Merge Cost  */
        Bool earlyDetectionSkip = false;
        xComputeCostMerge2Nx2N(m_MergeBestCU[uiDepth], m_MergeCU[uiDepth], &earlyDetectionSkip);
        
        if(!earlyDetectionSkip)
        {
            /*Compute 2Nx2N mode costs*/
            xComputeCostInter(m_InterCU_2Nx2N[uiDepth], SIZE_2Nx2N, 0);

            bTrySplitDQP = bTrySplit;

            if ((Int)uiDepth <= m_addSADDepth)
            {
                m_LCUPredictionSAD += m_temporalSAD;
                m_addSADDepth = uiDepth;
            }

            /*Compute Rect costs*/
            if (m_pcEncCfg->getUseRectInter())
            {
                xComputeCostInter(m_InterCU_Nx2N[uiDepth], SIZE_Nx2N, 1);
                xComputeCostInter(m_InterCU_2NxN[uiDepth], SIZE_2NxN, 2);
            }

            /*Choose best mode; initialise rpcBestCU to 2Nx2N*/
            rpcBestCU = m_InterCU_2Nx2N[uiDepth];

            YuvTemp = m_ppcPredYuvMode[0][uiDepth];
            m_ppcPredYuvMode[0][uiDepth] = m_ppcPredYuvBest[uiDepth];
            m_ppcPredYuvBest[uiDepth] = YuvTemp;
        
            if (m_InterCU_Nx2N[uiDepth]->getTotalCost() < rpcBestCU->getTotalCost())
            {
                rpcBestCU = m_InterCU_Nx2N[uiDepth];

                YuvTemp = m_ppcPredYuvMode[1][uiDepth];
                m_ppcPredYuvMode[1][uiDepth] = m_ppcPredYuvBest[uiDepth];
                m_ppcPredYuvBest[uiDepth] = YuvTemp;
            }
            if (m_InterCU_2NxN[uiDepth]->getTotalCost() < rpcBestCU->getTotalCost())
            {
                rpcBestCU = m_InterCU_2NxN[uiDepth];

                YuvTemp = m_ppcPredYuvMode[2][uiDepth];
                m_ppcPredYuvMode[2][uiDepth] = m_ppcPredYuvBest[uiDepth];
                m_ppcPredYuvBest[uiDepth] = YuvTemp;
            }

            m_pcPredSearch->encodeResAndCalcRdInterCU(rpcBestCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvBest[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcResiYuvBest[uiDepth], m_ppcRecoYuvBest[uiDepth], false);
        
            if(m_MergeBestCU[uiDepth]->getTotalCost() < rpcBestCU->getTotalCost())
            {
                rpcBestCU = m_MergeBestCU[uiDepth];
                YuvTemp = m_ppcPredYuvMode[3][uiDepth];
                m_ppcPredYuvMode[3][uiDepth] = m_ppcPredYuvBest[uiDepth];
                m_ppcPredYuvBest[uiDepth] = YuvTemp;
                m_ppcResiYuvBest[uiDepth]->clear();
                m_ppcPredYuvBest[uiDepth]->copyToPartYuv(m_ppcRecoYuvBest[uiDepth], 0);
            }

            /*compute intra cost */
            if (rpcBestCU->getCbf(0, TEXT_LUMA) != 0   ||
                rpcBestCU->getCbf(0, TEXT_CHROMA_U) != 0   ||
                rpcBestCU->getCbf(0, TEXT_CHROMA_V) != 0)
            {
                xComputeCostIntrainInter(m_IntraInInterCU[uiDepth], SIZE_2Nx2N);
                xEncodeIntrainInter(m_IntraInInterCU[uiDepth], m_ppcOrigYuv[uiDepth], m_ppcPredYuvMode[5][uiDepth], m_ppcResiYuvTemp[uiDepth],  m_ppcRecoYuvTemp[uiDepth]);

                if (m_IntraInInterCU[uiDepth]->getTotalCost() < rpcBestCU->getTotalCost())
                {
                    rpcBestCU = m_IntraInInterCU[uiDepth];

                    YuvTemp = m_ppcPredYuvMode[5][uiDepth];
                    m_ppcPredYuvMode[5][uiDepth] = m_ppcPredYuvBest[uiDepth];
                    m_ppcPredYuvBest[uiDepth] = YuvTemp;
                    TComYuv* tmpPic = m_ppcRecoYuvBest[uiDepth];
                    m_ppcRecoYuvBest[uiDepth] =  m_ppcRecoYuvTemp[uiDepth];
                    m_ppcRecoYuvTemp[uiDepth] = tmpPic;
                }
            }
        }
        else
        {
            rpcBestCU = m_MergeBestCU[uiDepth];
            YuvTemp = m_ppcPredYuvMode[3][uiDepth];
            m_ppcPredYuvMode[3][uiDepth] = m_ppcPredYuvBest[uiDepth];
            m_ppcPredYuvBest[uiDepth] = YuvTemp;
            m_ppcResiYuvBest[uiDepth]->clear();
            m_ppcPredYuvBest[uiDepth]->copyToPartYuv(m_ppcRecoYuvBest[uiDepth], 0);
        }


        /* Disable recursive analysis for whole CUs temporarily*/
        if ((rpcBestCU != 0) && (rpcBestCU->isSkipped(0)))
        {
#if CU_STAT_LOGFILE
            cntSkipCu[uiDepth]++;
#endif
            bSubBranch = false;
        }
        else
        {
            bSubBranch = true;
        }

        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeSplitFlag(rpcBestCU, 0, uiDepth, true);
        rpcBestCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
        rpcBestCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCost(rpcBestCU->getTotalDistortion(), rpcBestCU->getTotalBits());
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }
#if CU_STAT_LOGFILE
    if (rpcBestCU)
    {
        fprintf(fp1, "\n Width : %d ,Inter 2Nx2N_Merge : %d , 2Nx2N : %d , 2NxN : %d, Nx2N : %d , intra : %d", rpcBestCU->getWidth(0), m_MergeBestCU[uiDepth]->getTotalCost(), m_InterCU_2Nx2N[uiDepth]->getTotalCost(), m_InterCU_2NxN[uiDepth]->getTotalCost(), m_InterCU_Nx2N[uiDepth]->getTotalCost(), m_IntraInInterCU[uiDepth]->getTotalCost());
    }
#endif

// further split
    if (bSubBranch && bTrySplitDQP && uiDepth < g_maxCUDepth - g_addCUDepth)
    {
        rpcTempCU->initEstData(uiDepth, iQP);
        UChar       uhNextDepth         = (UChar)(uiDepth + 1);
        /*Best CU initialised to NULL; */
        TComDataCU* pcSubBestPartCU     = NULL;
        /*The temp structure is used for boundary analysis, and to copy Best SubCU mode data on return*/
        TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];

        for (UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++)
        {
            pcSubBestPartCU = NULL;
            pcSubTempPartCU->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.

            Bool bInSlice = pcSubTempPartCU->getSCUAddr() < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (pcSubTempPartCU->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (pcSubTempPartCU->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == uiPartUnitIdx) //initialize RD with previous depth buffer
                {
                    m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
                }
                else
                {
                    m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
                }
                xCompressInterCU(pcSubBestPartCU, pcSubTempPartCU, rpcTempCU, uhNextDepth, uiPartUnitIdx);

                /*Adding costs from best SUbCUs*/
                rpcTempCU->copyPartFrom(pcSubBestPartCU, uiPartUnitIdx, uhNextDepth, true); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(pcSubBestPartCU->getTotalNumPart() * uiPartUnitIdx, uhNextDepth);
            }
            else if (bInSlice)
            {
                pcSubTempPartCU->copyToPic((UChar)uhNextDepth);
                rpcTempCU->copyPartFrom(pcSubTempPartCU, uiPartUnitIdx, uhNextDepth, false);
            }
        }

        if (!bBoundary)
        {
            m_pcEntropyCoder->resetBits();
            m_pcEntropyCoder->encodeSplitFlag(rpcTempCU, 0, uiDepth, true);

            rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits();     // split bits
            rpcTempCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        }
        rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalDistortion(), rpcTempCU->getTotalBits());

        if ((g_maxCUWidth >> uiDepth) == rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() && rpcTempCU->getSlice()->getPPS()->getUseDQP())
        {
            Bool hasResidual = false;
            for (UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx++)
            {
                if (rpcTempCU->getCbf(uiBlkIdx, TEXT_LUMA) || rpcTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_U) || rpcTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_V))
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
                rpcTempCU->setQPSubCUs(rpcTempCU->getRefQP(uiTargetPartIdx), rpcTempCU, 0, uiDepth, foundNonZeroCbf);
                assert(foundNonZeroCbf);
            }
            else
            {
                rpcTempCU->setQPSubParts(rpcTempCU->getRefQP(uiTargetPartIdx), 0, uiDepth);     // set QP to default QP
            }
        }

        m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

#if  CU_STAT_LOGFILE
        if (rpcBestCU != 0)
        {
            if (rpcBestCU->getTotalCost() < rpcTempCU->getTotalCost())
            {
                if (rpcBestCU->getPredictionMode(0) == MODE_INTER)
                {
                    cntInter[uiDepth]++;
                    if (rpcBestCU->getPartitionSize(0) < 3)
                    {
                        cuInterDistribution[uiDepth][rpcBestCU->getPartitionSize(0)]++;
                    }
                    else
                    {
                        cuInterDistribution[uiDepth][3]++;
                    }
                }
                else if (rpcBestCU->getPredictionMode(0) == MODE_INTRA)
                {
                    cntIntra[uiDepth]++;
                    if (rpcBestCU->getLumaIntraDir()[0] > 1)
                    {
                        cuIntraDistribution[uiDepth][2]++;
                    }
                    else
                    {
                        cuIntraDistribution[uiDepth][rpcBestCU->getLumaIntraDir()[0]]++;
                    }
                }
            }
            else
            {
                cntSplit[uiDepth]++;
            }
        }
#endif // if  LOGGING

        /*If Best Mode is not NULL; then compare costs. Else assign best mode to Sub-CU costs
        Copy Recon data from Temp structure to Best structure*/
        if (rpcBestCU)
        {
            if (rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost())
            {
                rpcBestCU = rpcTempCU;

                YuvTemp = m_ppcRecoYuvTemp[uiDepth];
                m_ppcRecoYuvTemp[uiDepth] = m_ppcRecoYuvBest[uiDepth];
                m_ppcRecoYuvBest[uiDepth] = YuvTemp;
            }
        }
        else
        {
            rpcBestCU = rpcTempCU;
            YuvTemp = m_ppcRecoYuvTemp[uiDepth];
            m_ppcRecoYuvTemp[uiDepth] = m_ppcRecoYuvBest[uiDepth];
            m_ppcRecoYuvBest[uiDepth] = YuvTemp;
        }
    }

#if CU_STAT_LOGFILE
    if (uiDepth == 3)
    {
        if (!rpcBestCU->isSkipped(0))
        {
            if (rpcBestCU->getPredictionMode(0) == MODE_INTER)
            {
                cntInter[uiDepth]++;
                if (rpcBestCU->getPartitionSize(0) < 3)
                {
                    cuInterDistribution[uiDepth][rpcBestCU->getPartitionSize(0)]++;
                }
                else
                {
                    cuInterDistribution[uiDepth][3]++;
                }
            }
            else if (rpcBestCU->getPredictionMode(0) == MODE_INTRA)
            {
                cntIntra[uiDepth]++;
                if (rpcBestCU->getPartitionSize(0) == SIZE_2Nx2N)
                {
                    if (rpcBestCU->getLumaIntraDir()[0] > 1)
                    {
                        cuIntraDistribution[uiDepth][2]++;
                    }
                    else
                    {
                        cuIntraDistribution[uiDepth][rpcBestCU->getLumaIntraDir()[0]]++;
                    }
                }
                else if (rpcBestCU->getPartitionSize(0) == SIZE_NxN)
                {
                    cntIntraNxN++;
                }
            }
        }
    }
#endif // if LOGGING
       /* Copy Best data to Picture for next partition prediction.*/
    rpcBestCU->copyToPic((UChar)uiDepth);
    /* Copy Yuv data to picture Yuv*/
    xCopyYuv2Pic(rpcBestCU->getPic(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU(), uiDepth, uiDepth, rpcBestCU, uiLPelX, uiTPelY);

    if (bBoundary || (bSliceEnd && bInsidePicture))
    {
        return;
    }

    /* Assert if Best prediction mode is NONE
     Selected mode's RD-cost must be not MAX_DOUBLE.*/
    assert(rpcBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(rpcBestCU->getPredictionMode(0) != MODE_NONE);
    assert(rpcBestCU->getTotalCost() != MAX_DOUBLE);
}

#if _MSC_VER
#pragma warning (default: 4244)
#pragma warning (disable: 4018)
#endif
