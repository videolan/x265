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

#if _MSC_VER
#pragma warning (disable: 4244)
#pragma warning (disable: 4018)
#endif

using namespace x265;
#if CU_STAT_LOGFILE
extern int totalCU;
extern int cntInter[4], cntIntra[4], cntSplit[4], cntIntraNxN;
extern  int cuInterDistribution[4][4], cuIntraDistribution[4][3];
extern  int cntSkipCu[4],  cntTotalCu[4];
extern FILE* fp1;

#endif

Void TEncCu::xComputeCostIntrainInter(TComDataCU*& rpcTempCU, PartSize eSize, UInt index)
{
    UInt uiDepth = rpcTempCU->getDepth(0);

    //PPAScopeEvent(TEncCU_xCheckRDCostIntra + uiDepth);

    rpcTempCU->setSkipFlagSubParts(false, 0, uiDepth);

    rpcTempCU->setPartSizeSubParts(eSize, 0, uiDepth);
    rpcTempCU->setPredModeSubParts(MODE_INTRA, 0, uiDepth);
    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(), 0, uiDepth);

    Bool bSeparateLumaChroma = true; // choose estimation mode
    UInt uiPreCalcDistC      = 0;
    if (!bSeparateLumaChroma)
    {
        m_pcPredSearch->preestChromaPredMode(rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvMode[index][uiDepth]);
    }
    m_pcPredSearch->estIntraPredQT(rpcTempCU, m_ppcOrigYuv[uiDepth],  m_ppcPredYuvMode[index][uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], uiPreCalcDistC, bSeparateLumaChroma);

    m_ppcRecoYuvTemp[uiDepth]->copyToPicLuma(rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getAddr(), rpcTempCU->getZorderIdxInCU());

    m_pcPredSearch->estIntraPredChromaQT(rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvMode[index][uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], uiPreCalcDistC);

    m_pcEntropyCoder->resetBits();
    if (rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_pcEntropyCoder->encodeCUTransquantBypassFlag(rpcTempCU, 0, true);
    }
    m_pcEntropyCoder->encodeSkipFlag(rpcTempCU, 0,          true);
    m_pcEntropyCoder->encodePredMode(rpcTempCU, 0,          true);
    m_pcEntropyCoder->encodePartSize(rpcTempCU, 0, uiDepth, true);
    m_pcEntropyCoder->encodePredInfo(rpcTempCU, 0,          true);
    m_pcEntropyCoder->encodeIPCMInfo(rpcTempCU, 0, true);

    // Encode Coefficients
    Bool bCodeDQP = getdQPFlag();
    // m_pcEntropyCoder->encodeCoeff(rpcTempCU, 0, uiDepth, rpcTempCU->getWidth(0), rpcTempCU->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

    rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
    rpcTempCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
#if FAST_MODE_DECISION
    UInt partEnum = PartitionFromSizes(rpcTempCU->getWidth(0), rpcTempCU->getHeight(0));
    UInt SATD = primitives.satd[partEnum]((pixel*)m_ppcOrigYuv[uiDepth]->getLumaAddr(), m_ppcOrigYuv[uiDepth]->getStride(),
                                          (pixel*)m_ppcPredYuvMode[index][uiDepth]->getLumaAddr(),  m_ppcPredYuvMode[index][uiDepth]->getStride());
    x265_emms();
    rpcTempCU->getTotalDistortion() = SATD;
#endif
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalDistortion(), rpcTempCU->getTotalBits());

    //xCheckDQP(rpcTempCU);
    //xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth);
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
    for (UInt ui = 0; ui < numValidMergeCand; ++ui)
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

    for (UInt uiNoResidual = 0; uiNoResidual < iteration; ++uiNoResidual)
    {
        for (UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand)
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
#if 0
                    m_pcPredSearch->encodeResAndCalcRdInterCU(rpcTempCU,
                                                              m_ppcOrigYuv[uhDepth],
                                                              m_ppcPredYuvTemp[uhDepth],
                                                              m_ppcResiYuvTemp[uhDepth],
                                                              m_ppcResiYuvBest[uhDepth],
                                                              m_ppcRecoYuvTemp[uhDepth],
                                                              (uiNoResidual ? true : false));
#endif
                    /*Todo: Fix the satd cost estimates. Why is merge being chosen in high motion areas: estimated distortion is too low?*/

                    UInt partEnum = PartitionFromSizes(rpcTempCU->getWidth(0), rpcTempCU->getHeight(0));
                    UInt SATD = primitives.satd[partEnum]((pixel*)m_ppcOrigYuv[uhDepth]->getLumaAddr(), m_ppcOrigYuv[uhDepth]->getStride(),
                                                          (pixel*)m_ppcPredYuvTemp[uhDepth]->getLumaAddr(), m_ppcPredYuvTemp[uhDepth]->getStride());
                    x265_emms();
                    rpcTempCU->getTotalDistortion() = SATD;
                    rpcTempCU->getTotalCost() = SATD;

                    if (uiNoResidual == 0)
                    {
                        if (rpcTempCU->getQtRootCbf(0) == 0)
                        {
                            mergeCandBuffer[uiMergeCand] = 1;
                        }
                    }

                    rpcTempCU->setSkipFlagSubParts(rpcTempCU->getQtRootCbf(0) == 0, 0, uhDepth);
                    Int orgQP = rpcTempCU->getQP(0);

                    //xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth);
                    if (rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost())
                    {
                        TComDataCU* tmp = rpcTempCU;
                        rpcTempCU = rpcBestCU;
                        rpcBestCU = tmp;
                        // Change Prediction data
                        TComYuv* pcYuv = NULL;
                        pcYuv =  m_ppcPredYuvMode[3][uhDepth];
                        m_ppcPredYuvMode[3][uhDepth]  = m_ppcPredYuvMode[4][uhDepth];
                        m_ppcPredYuvMode[4][uhDepth] = pcYuv;
                    }

                    rpcTempCU->initEstData(uhDepth, orgQP);

                    if (m_pcEncCfg->getUseFastDecisionForMerge() && !bestIsSkip)
                    {
                        bestIsSkip = rpcTempCU->getQtRootCbf(0) == 0;
                    }
                }
            }
        }

        if (uiNoResidual == 0 && m_pcEncCfg->getUseEarlySkipDetection())
        {
            if (rpcTempCU->getQtRootCbf(0) == 0)
            {
                if (rpcTempCU->getMergeFlag(0))
                {
                    *earlyDetectionSkipMode = true;
                }
                else
                {
                    Int absoulte_MV = 0;
                    for (UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++)
                    {
                        if (rpcTempCU->getSlice()->getNumRefIdx(RefPicList(uiRefListIdx)) > 0)
                        {
                            TComCUMvField* pcCUMvField = rpcTempCU->getCUMvField(RefPicList(uiRefListIdx));
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
    Bool earlyDetectionSkipMode = false;

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
            m_IntrainInterCU[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_MergeCU[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_MergeBestCU[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
        }
        else
        {
            m_InterCU_2Nx2N[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_InterCU_2NxN[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_InterCU_Nx2N[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_IntrainInterCU[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_MergeCU[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
            m_MergeBestCU[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP);
        }

        /*Compute  Merge Cost  */
        xComputeCostMerge2Nx2N(m_MergeBestCU[uiDepth], m_MergeCU[uiDepth], &earlyDetectionSkipMode);
        rpcBestCU = m_MergeBestCU[uiDepth];
        YuvTemp = m_ppcPredYuvMode[3][uiDepth];
        m_ppcPredYuvMode[3][uiDepth] = m_ppcPredYuvBest[uiDepth];
        m_ppcPredYuvBest[uiDepth] = YuvTemp;

        /*Compute 2Nx2N mode costs*/
        xComputeCostInter(m_InterCU_2Nx2N[uiDepth], SIZE_2Nx2N, 0);

        bTrySplitDQP = bTrySplit;

        if (uiDepth <= m_addSADDepth)
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

        /* Disable recursive analysis for whole CUs temporarily*/
        bSubBranch = false;

        /*Choose best mode; initialise rpcBestCU to 2Nx2N*/
        if (m_InterCU_2Nx2N[uiDepth]->getTotalCost() < rpcBestCU->getTotalCost())
        {
            rpcBestCU = m_InterCU_2Nx2N[uiDepth];

            YuvTemp = m_ppcPredYuvMode[0][uiDepth];
            m_ppcPredYuvMode[0][uiDepth] = m_ppcPredYuvBest[uiDepth];
            m_ppcPredYuvBest[uiDepth] = YuvTemp;
        }
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

        /* Perform encode residual for the best mode chosen only*/
        m_pcPredSearch->encodeResAndCalcRdInterCU(rpcBestCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvBest[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcResiYuvBest[uiDepth], m_ppcRecoYuvBest[uiDepth], false);
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }

    if (rpcBestCU != 0 && rpcBestCU->isSkipped(0))
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

// further split
    if (bSubBranch && bTrySplitDQP && uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth)
    {
        rpcTempCU->initEstData(uiDepth, iQP);
        UChar       uhNextDepth         = uiDepth + 1;
        /*Best CU initialised to NULL; */
        TComDataCU* pcSubBestPartCU     = NULL;
        /*The temp structure is used for boundary analysis, and to copy Best SubCU mode data on return*/
        TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];

        rpcTempCU->getTotalCost() = 0;
        for (UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++)
        {
            pcSubBestPartCU = NULL;
            pcSubTempPartCU->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.

            Bool bInSlice = pcSubTempPartCU->getSCUAddr() < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (pcSubTempPartCU->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (pcSubTempPartCU->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                xCompressInterCU(pcSubBestPartCU, pcSubTempPartCU, rpcTempCU, uhNextDepth, uiPartUnitIdx);

                /*Adding costs from best SUbCUs*/
                rpcTempCU->getTotalCost() += pcSubBestPartCU->getTotalCost();
                rpcTempCU->copyPartFrom(pcSubBestPartCU, uiPartUnitIdx, uhNextDepth, false); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(pcSubBestPartCU->getTotalNumPart() * uiPartUnitIdx, uhNextDepth);
            }
            else if (bInSlice)
            {
                pcSubTempPartCU->copyToPic(uhNextDepth);
                rpcTempCU->copyPartFrom(pcSubTempPartCU, uiPartUnitIdx, uhNextDepth, false);        
            }
        }

        if ((g_uiMaxCUWidth >> uiDepth) == rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() && rpcTempCU->getSlice()->getPPS()->getUseDQP())
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
#if  CU_STAT_LOGFILE
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
#endif // if  LOGGING
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
    rpcBestCU->copyToPic(uiDepth);
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