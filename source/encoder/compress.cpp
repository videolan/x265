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

Void TEncCu::xEncodeIntraInInter(TComDataCU*& cu, TComYuv* fencYuv, TComYuv* predYuv,  TShortYUV*& outResiYuv, TComYuv*& outReconYuv)
{
    UInt64   dPUCost = 0;
    UInt   uiPUDistY = 0;
    UInt   uiPUDistC = 0;
    UInt   depth = cu->getDepth(0);
    UInt    uiInitTrDepth  = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;

    // set context models
    m_search->m_rdGoOnSbacCoder->load(m_search->m_rdSbacCoders[depth][CI_CURR_BEST]);

    m_search->xRecurIntraCodingQT(cu, uiInitTrDepth, 0, true, fencYuv, predYuv, outResiYuv, uiPUDistY, uiPUDistC, false, dPUCost);
    m_search->xSetIntraResultQT(cu, uiInitTrDepth, 0, true, outReconYuv);

    //=== update PU data ====
    cu->copyToPic(cu->getDepth(0), 0, uiInitTrDepth);

    //===== set distortion (rate and r-d costs are determined later) =====
    cu->getTotalDistortion() = uiPUDistY + uiPUDistC;

    outReconYuv->copyToPicLuma(cu->getPic()->getPicYuvRec(), cu->getAddr(), cu->getZorderIdxInCU());

    m_search->estIntraPredChromaQT(cu, fencYuv, predYuv, outResiYuv, outReconYuv, uiPUDistC);
    m_entropyCoder->resetBits();
    if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
    }
    m_entropyCoder->encodeSkipFlag(cu, 0,          true);
    m_entropyCoder->encodePredMode(cu, 0,          true);
    m_entropyCoder->encodePartSize(cu, 0, depth, true);
    m_entropyCoder->encodePredInfo(cu, 0,          true);
    m_entropyCoder->encodeIPCMInfo(cu, 0, true);

    // Encode Coefficients
    Bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(cu, 0, depth, cu->getWidth(0), cu->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    cu->getTotalBits() = m_entropyCoder->getNumberOfWrittenBits();
    cu->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    cu->getTotalCost() = m_rdCost->calcRdCost(cu->getTotalDistortion(), cu->getTotalBits());
}

Void TEncCu::xComputeCostIntraInInter(TComDataCU*& cu, PartSize partSize)
{
    UInt    depth        = cu->getDepth(0);

    cu->setSkipFlagSubParts(false, 0, depth);
    cu->setPartSizeSubParts(partSize, 0, depth);
    cu->setPredModeSubParts(MODE_INTRA, 0, depth);
    cu->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    UInt    uiInitTrDepth  = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    UInt    width        = cu->getWidth(0) >> uiInitTrDepth;
    UInt    uiWidthBit     = cu->getIntraSizeIdx(0);
    UInt64  CandCostList[FAST_UDI_MAX_RDMODE_NUM];
    UInt    CandNum;

    UInt partOffset = 0;

    //===== init pattern for luma prediction =====
    cu->getPattern()->initPattern(cu, uiInitTrDepth, partOffset);
    // Reference sample smoothing
    cu->getPattern()->initAdiPattern(cu, partOffset, uiInitTrDepth, m_search->getPredicBuf(),  m_search->getPredicBufWidth(),  m_search->getPredicBufHeight(), m_search->refAbove, m_search->refLeft, m_search->refAboveFlt, m_search->refLeftFlt);

    //===== determine set of modes to be tested (using prediction signal only) =====
    UInt numModesAvailable = 35; //total number of Intra modes
    Pel* piOrg         = m_origYuv[depth]->getLumaAddr(0, width);
    Pel* pred        = m_modePredYuv[5][depth]->getLumaAddr(0, width);
    UInt stride      = m_modePredYuv[5][depth]->getStride();
    UInt uiRdModeList[FAST_UDI_MAX_RDMODE_NUM];
    UInt numModesForFullRD = g_intraModeNumFast[uiWidthBit];
    Int nLog2SizeMinus2 = g_convertToBit[width];
    x265::pixelcmp_t sa8d = x265::primitives.sa8d[nLog2SizeMinus2];
    {
        assert(numModesForFullRD < numModesAvailable);

        for (UInt i = 0; i < numModesForFullRD; i++)
        {
            CandCostList[i] = MAX_INT64;
        }

        CandNum = 0;
        UInt uiSads[35];
        Bool bFilter = (width <= 16);
        Pel *ptrSrc = m_search->getPredicBuf();

        // 1
        primitives.intra_pred_dc((pixel*)ptrSrc + ADI_BUF_STRIDE + 1, ADI_BUF_STRIDE, (pixel*)pred, stride, width, bFilter);
        uiSads[DC_IDX] = sa8d((pixel*)piOrg, stride, (pixel*)pred, stride);

        // 0
        if (width >= 8 && width <= 32)
        {
            ptrSrc += ADI_BUF_STRIDE * (2 * width + 1);
        }
        primitives.intra_pred_planar((pixel*)ptrSrc + ADI_BUF_STRIDE + 1, ADI_BUF_STRIDE, (pixel*)pred, stride, width);
        uiSads[PLANAR_IDX] = sa8d((pixel*)piOrg, stride, (pixel*)pred, stride);

        // 33 Angle modes once
        if (width <= 16)
        {
            ALIGN_VAR_32(Pel, buf1[MAX_CU_SIZE * MAX_CU_SIZE]);
            ALIGN_VAR_32(Pel, tmp[33 * MAX_CU_SIZE * MAX_CU_SIZE]);

            // Transpose NxN
            x265::primitives.transpose[nLog2SizeMinus2]((pixel*)buf1, (pixel*)piOrg, stride);

            Pel *pAbove0 = m_search->refAbove    + width - 1;
            Pel *pAbove1 = m_search->refAboveFlt + width - 1;
            Pel *pLeft0  = m_search->refLeft     + width - 1;
            Pel *pLeft1  = m_search->refLeftFlt  + width - 1;

            x265::primitives.intra_pred_allangs[nLog2SizeMinus2]((pixel*)tmp, (pixel*)pAbove0, (pixel*)pLeft0, (pixel*)pAbove1, (pixel*)pLeft1, (width <= 16));

            // TODO: We need SATD_x4 here
            for (UInt mode = 2; mode < numModesAvailable; mode++)
            {
                bool modeHor = (mode < 18);
                Pel *src = (modeHor ? buf1 : piOrg);
                intptr_t srcStride = (modeHor ? width : stride);

                // use hadamard transform here
                UInt uiSad = sa8d((pixel*)src, srcStride, (pixel*)&tmp[(mode - 2) * (width * width)], width);
                uiSads[mode] = uiSad;
            }
        }
        else
        {
            for (UInt mode = 2; mode < numModesAvailable; mode++)
            {
                m_search->predIntraLumaAng(cu->getPattern(), mode, pred, stride, width);

                // use hadamard transform here
                UInt uiSad = sa8d((pixel*)piOrg, stride, (pixel*)pred, stride);
                uiSads[mode] = uiSad;
            }
        }

        for (UInt mode = 0; mode < numModesAvailable; mode++)
        {
            UInt uiSad = uiSads[mode];
            UInt iModeBits = m_search->xModeBitsIntra(cu, mode, 0, partOffset, depth, uiInitTrDepth);
            UInt64 cost = m_rdCost->calcRdSADCost(uiSad, iModeBits);
            CandNum += m_search->xUpdateCandList(mode, cost, numModesForFullRD, uiRdModeList, CandCostList);      //Find N least cost  modes. N = numModesForFullRD
        }

        Int uiPreds[3] = { -1, -1, -1 };
        Int iMode = -1;
        Int numCand = cu->getIntraDirLumaPredictor(partOffset, uiPreds, &iMode);
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

    cu->setLumaIntraDirSubParts(uiOrgMode, partOffset, depth + uiInitTrDepth);

    // set context models
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
}

/** check RD costs for a CU block encoded with merge
 * \param outBestCU
 * \param outTempCU
 * \returns Void
 */
Void TEncCu::xComputeCostInter(TComDataCU*& outTempCU, PartSize partSize, UInt Index, Bool bUseMRG)
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

    m_search->predInterSearch(outTempCU, m_origYuv[uhDepth], m_modePredYuv[Index][uhDepth], bUseMRG);
}

Void TEncCu::xCompressInterCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU*& cu, UInt depth, UInt PartitionIndex)
{
#if CU_STAT_LOGFILE
    cntTotalCu[depth]++;
#endif
    m_abortFlag = false;

    TComPic* pcPic = outTempCU->getPic();

    // get Original YUV data from picture
    m_origYuv[depth]->copyFromPicYuv(pcPic->getPicYuvOrg(), outTempCU->getAddr(), outTempCU->getZorderIdxInCU());

    // variables for fast encoder decision
    Bool    bTrySplit    = true;

    // variable for Early CU determination
    Bool    bSubBranch = true;

    Bool bTrySplitDQP  = true;

    Bool bBoundary = false;
    UInt uiLPelX   = outTempCU->getCUPelX();
    UInt uiRPelX   = uiLPelX + outTempCU->getWidth(0)  - 1;
    UInt uiTPelY   = outTempCU->getCUPelY();
    UInt uiBPelY   = uiTPelY + outTempCU->getHeight(0) - 1;

    Int iQP = m_cfg->getUseRateCtrl() ? m_rateControl->getRCQP() : outTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * pcSlice = outTempCU->getPic()->getSlice();
    Bool bSliceEnd = (pcSlice->getSliceCurEndCUAddr() > outTempCU->getSCUAddr() && pcSlice->getSliceCurEndCUAddr() < outTempCU->getSCUAddr() + outTempCU->getTotalNumPart());
    Bool bInsidePicture = (uiRPelX < outTempCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < outTempCU->getSlice()->getSPS()->getPicHeightInLumaSamples());
    // We need to split, so don't try these modes.
    TComYuv* YuvTemp = NULL;
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit    = true;

        /*Initialise all Mode-CUs based on parentCU*/
        if (depth == 0)
        {
            m_interCU_2Nx2N[depth]->initCU(cu->getPic(), cu->getAddr());
            m_interCU_Nx2N[depth]->initCU(cu->getPic(), cu->getAddr());
            m_interCU_2NxN[depth]->initCU(cu->getPic(), cu->getAddr());
            m_intraInInterCU[depth]->initCU(cu->getPic(), cu->getAddr());
            m_mergeCU[depth]->initCU(cu->getPic(), cu->getAddr());
            m_bestMergeCU[depth]->initCU(cu->getPic(), cu->getAddr());
        }
        else
        {
            m_interCU_2Nx2N[depth]->initSubCU(cu, PartitionIndex, depth, iQP);
            m_interCU_2NxN[depth]->initSubCU(cu, PartitionIndex, depth, iQP);
            m_interCU_Nx2N[depth]->initSubCU(cu, PartitionIndex, depth, iQP);
            m_intraInInterCU[depth]->initSubCU(cu, PartitionIndex, depth, iQP);
            m_mergeCU[depth]->initSubCU(cu, PartitionIndex, depth, iQP);
            m_bestMergeCU[depth]->initSubCU(cu, PartitionIndex, depth, iQP);
        }

        /*Compute  Merge Cost  */
        Bool earlyDetectionSkip = false;
        xCheckRDCostMerge2Nx2N(m_bestMergeCU[depth], m_mergeCU[depth], &earlyDetectionSkip, m_modePredYuv[3][depth], m_bestMergeRecoYuv[depth]);

        if (!earlyDetectionSkip)
        {
            /*Compute 2Nx2N mode costs*/
            xComputeCostInter(m_interCU_2Nx2N[depth], SIZE_2Nx2N, 0);

            bTrySplitDQP = bTrySplit;

            if ((Int)depth <= m_addSADDepth)
            {
                m_LCUPredictionSAD += m_temporalSAD;
                m_addSADDepth = depth;
            }

            /*Compute Rect costs*/
            if (m_cfg->getUseRectInter())
            {
                xComputeCostInter(m_interCU_Nx2N[depth], SIZE_Nx2N, 1);
                xComputeCostInter(m_interCU_2NxN[depth], SIZE_2NxN, 2);
            }

            /*Choose best mode; initialise outBestCU to 2Nx2N*/
            outBestCU = m_interCU_2Nx2N[depth];

            YuvTemp = m_modePredYuv[0][depth];
            m_modePredYuv[0][depth] = m_bestPredYuv[depth];
            m_bestPredYuv[depth] = YuvTemp;

            if (m_interCU_Nx2N[depth]->getTotalCost() < outBestCU->getTotalCost())
            {
                outBestCU = m_interCU_Nx2N[depth];

                YuvTemp = m_modePredYuv[1][depth];
                m_modePredYuv[1][depth] = m_bestPredYuv[depth];
                m_bestPredYuv[depth] = YuvTemp;
            }
            if (m_interCU_2NxN[depth]->getTotalCost() < outBestCU->getTotalCost())
            {
                outBestCU = m_interCU_2NxN[depth];

                YuvTemp = m_modePredYuv[2][depth];
                m_modePredYuv[2][depth] = m_bestPredYuv[depth];
                m_bestPredYuv[depth] = YuvTemp;
            }

            m_search->encodeResAndCalcRdInterCU(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth], m_bestResiYuv[depth], m_bestRecoYuv[depth], false);

            if (m_bestMergeCU[depth]->getTotalCost() < outBestCU->getTotalCost())
            {
                outBestCU = m_bestMergeCU[depth];
                YuvTemp = m_modePredYuv[3][depth];
                m_modePredYuv[3][depth] = m_bestPredYuv[depth];
                m_bestPredYuv[depth] = YuvTemp;
                
                YuvTemp = m_bestRecoYuv[depth];
                m_bestRecoYuv[depth] = m_bestMergeRecoYuv[depth];
                m_bestMergeRecoYuv[depth] = YuvTemp;
            }

            /*compute intra cost */
            if (outBestCU->getCbf(0, TEXT_LUMA) != 0   ||
                outBestCU->getCbf(0, TEXT_CHROMA_U) != 0   ||
                outBestCU->getCbf(0, TEXT_CHROMA_V) != 0)
            {
                xComputeCostIntraInInter(m_intraInInterCU[depth], SIZE_2Nx2N);
                xEncodeIntraInInter(m_intraInInterCU[depth], m_origYuv[depth], m_modePredYuv[5][depth], m_tmpResiYuv[depth],  m_tmpRecoYuv[depth]);

                if (m_intraInInterCU[depth]->getTotalCost() < outBestCU->getTotalCost())
                {
                    outBestCU = m_intraInInterCU[depth];

                    YuvTemp = m_modePredYuv[5][depth];
                    m_modePredYuv[5][depth] = m_bestPredYuv[depth];
                    m_bestPredYuv[depth] = YuvTemp;
                    TComYuv* tmpPic = m_bestRecoYuv[depth];
                    m_bestRecoYuv[depth] =  m_tmpRecoYuv[depth];
                    m_tmpRecoYuv[depth] = tmpPic;
                }
            }
        }
        else
        {
            outBestCU = m_bestMergeCU[depth];
            YuvTemp = m_modePredYuv[3][depth];
            m_modePredYuv[3][depth] = m_bestPredYuv[depth];
            m_bestPredYuv[depth] = YuvTemp;
            
            YuvTemp = m_bestRecoYuv[depth];
            m_bestRecoYuv[depth] = m_bestMergeRecoYuv[depth];
            m_bestMergeRecoYuv[depth] = YuvTemp;            
        }

        /* Disable recursive analysis for whole CUs temporarily*/
        if ((outBestCU != 0) && (outBestCU->isSkipped(0)))
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

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth, true);
        outBestCU->getTotalBits() += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        outBestCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        outBestCU->getTotalCost()  = m_rdCost->calcRdCost(outBestCU->getTotalDistortion(), outBestCU->getTotalBits());
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }
#if CU_STAT_LOGFILE
    if (outBestCU)
    {
        fprintf(fp1, "\n Width : %d ,Inter 2Nx2N_Merge : %d , 2Nx2N : %d , 2NxN : %d, Nx2N : %d , intra : %d", outBestCU->getWidth(0), m_bestMergeCU[depth]->getTotalCost(), m_interCU_2Nx2N[depth]->getTotalCost(), m_interCU_2NxN[depth]->getTotalCost(), m_interCU_Nx2N[depth]->getTotalCost(), m_intraInInterCU[depth]->getTotalCost());
    }
#endif

// further split
    if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
    {
        outTempCU->initEstData(depth, iQP);
        UChar       uhNextDepth         = (UChar)(depth + 1);
        /*Best CU initialised to NULL; */
        TComDataCU* pcSubBestPartCU     = NULL;
        /*The temp structure is used for boundary analysis, and to copy Best SubCU mode data on return*/
        TComDataCU* pcSubTempPartCU     = m_tempCU[uhNextDepth];

        for (UInt partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
        {
            pcSubBestPartCU = NULL;
            pcSubTempPartCU->initSubCU(outTempCU, partUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.

            Bool bInSlice = pcSubTempPartCU->getSCUAddr() < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (pcSubTempPartCU->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (pcSubTempPartCU->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[uhNextDepth][CI_NEXT_BEST]);
                }
                xCompressInterCU(pcSubBestPartCU, pcSubTempPartCU, outTempCU, uhNextDepth, partUnitIdx);

                /*Adding costs from best SUbCUs*/
                outTempCU->copyPartFrom(pcSubBestPartCU, partUnitIdx, uhNextDepth, true); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(pcSubBestPartCU->getTotalNumPart() * partUnitIdx, uhNextDepth);
            }
            else if (bInSlice)
            {
                pcSubTempPartCU->copyToPic((UChar)uhNextDepth);
                outTempCU->copyPartFrom(pcSubTempPartCU, partUnitIdx, uhNextDepth, false);
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
        if (outBestCU != 0)
        {
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
        }
#endif // if  LOGGING

        /*If Best Mode is not NULL; then compare costs. Else assign best mode to Sub-CU costs
        Copy Recon data from Temp structure to Best structure*/
        if (outBestCU)
        {
            if (outTempCU->getTotalCost() < outBestCU->getTotalCost())
            {
                outBestCU = outTempCU;

                YuvTemp = m_tmpRecoYuv[depth];
                m_tmpRecoYuv[depth] = m_bestRecoYuv[depth];
                m_bestRecoYuv[depth] = YuvTemp;
            }
        }
        else
        {
            outBestCU = outTempCU;
            YuvTemp = m_tmpRecoYuv[depth];
            m_tmpRecoYuv[depth] = m_bestRecoYuv[depth];
            m_bestRecoYuv[depth] = YuvTemp;
        }
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
       /* Copy Best data to Picture for next partition prediction.*/
    outBestCU->copyToPic((UChar)depth);
    /* Copy Yuv data to picture Yuv*/
    xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, uiLPelX, uiTPelY);

    if (bBoundary || (bSliceEnd && bInsidePicture))
    {
        return;
    }

    /* Assert if Best prediction mode is NONE
     Selected mode's RD-cost must be not MAX_DOUBLE.*/
    assert(outBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(outBestCU->getPredictionMode(0) != MODE_NONE);
    assert(outBestCU->getTotalCost() != MAX_DOUBLE);
}

#if _MSC_VER
#pragma warning (default: 4244)
#pragma warning (disable: 4018)
#endif
