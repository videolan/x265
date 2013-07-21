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

Void TEncCu::xEncodeIntraInInter(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv,  TShortYUV* outResiYuv, TComYuv* outReconYuv)
{
    UInt64 dPUCost = 0;
    UInt puDistY = 0;
    UInt puDistC = 0;
    UInt depth = cu->getDepth(0);
    UInt initTrDepth = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;

    // set context models
    m_search->m_rdGoOnSbacCoder->load(m_search->m_rdSbacCoders[depth][CI_CURR_BEST]);

    m_search->xRecurIntraCodingQT(cu, initTrDepth, 0, true, fencYuv, predYuv, outResiYuv, puDistY, puDistC, false, dPUCost);
    m_search->xSetIntraResultQT(cu, initTrDepth, 0, true, outReconYuv);

    //=== update PU data ====
    cu->copyToPic(cu->getDepth(0), 0, initTrDepth);

    //===== set distortion (rate and r-d costs are determined later) =====
    cu->m_totalDistortion = puDistY + puDistC;

    outReconYuv->copyToPicLuma(cu->getPic()->getPicYuvRec(), cu->getAddr(), cu->getZorderIdxInCU());

    m_search->estIntraPredChromaQT(cu, fencYuv, predYuv, outResiYuv, outReconYuv, puDistC);
    m_entropyCoder->resetBits();
    if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(cu, 0, true);
    }
    m_entropyCoder->encodeSkipFlag(cu, 0, true);
    m_entropyCoder->encodePredMode(cu, 0, true);
    m_entropyCoder->encodePartSize(cu, 0, depth, true);
    m_entropyCoder->encodePredInfo(cu, 0, true);
    m_entropyCoder->encodeIPCMInfo(cu, 0, true);

    // Encode Coefficients
    Bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(cu, 0, depth, cu->getWidth(0), cu->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    cu->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();
    cu->m_totalBins = ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    cu->m_totalCost = m_rdCost->calcRdCost(cu->m_totalDistortion, cu->m_totalBits);
}

Void TEncCu::xComputeCostIntraInInter(TComDataCU*& cu, PartSize partSize)
{
    UInt depth = cu->getDepth(0);

    cu->setSkipFlagSubParts(false, 0, depth);
    cu->setPartSizeSubParts(partSize, 0, depth);
    cu->setPredModeSubParts(MODE_INTRA, 0, depth);
    cu->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    UInt initTrDepth = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    UInt width       = cu->getWidth(0) >> initTrDepth;
    UInt uiWidthBit  = cu->getIntraSizeIdx(0);
    UInt64 CandCostList[FAST_UDI_MAX_RDMODE_NUM];
    UInt CandNum;
    UInt partOffset = 0;

    //===== init pattern for luma prediction =====
    cu->getPattern()->initPattern(cu, initTrDepth, partOffset);
    // Reference sample smoothing
    cu->getPattern()->initAdiPattern(cu, partOffset, initTrDepth, m_search->getPredicBuf(),  m_search->getPredicBufWidth(),  m_search->getPredicBufHeight(), m_search->refAbove, m_search->refLeft, m_search->refAboveFlt, m_search->refLeftFlt);

    //===== determine set of modes to be tested (using prediction signal only) =====
    UInt numModesAvailable = 35; //total number of Intra modes
    Pel* fenc   = m_origYuv[depth]->getLumaAddr(0, width);
    Pel* pred   = m_modePredYuv[5][depth]->getLumaAddr(0, width);
    UInt stride = m_modePredYuv[5][depth]->getStride();
    UInt rdModeList[FAST_UDI_MAX_RDMODE_NUM];
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
        UInt modeCosts[35];

        // CHM: TODO - The code isn't copy from TEncSearch::estIntraPredQT by me, I sync it, please check its logic
        Pel *pAbove0 = m_search->refAbove    + width - 1;
        Pel *pAbove1 = m_search->refAboveFlt + width - 1;
        Pel *pLeft0  = m_search->refLeft     + width - 1;
        Pel *pLeft1  = m_search->refLeftFlt  + width - 1;

        // 33 Angle modes once
        ALIGN_VAR_32(Pel, buf_trans[32 * 32]);
        ALIGN_VAR_32(Pel, tmp[33 * 32 * 32]);

        if (width <= 32)
        {
            // 1
            primitives.intra_pred_dc(pAbove0 + 1, pLeft0 + 1, pred, stride, width, (width <= 16));
            modeCosts[DC_IDX] = sa8d(fenc, stride, pred, stride);

            // 0
            Pel *above   = pAbove0;
            Pel *left    = pLeft0;
            if (width >= 8 && width <= 32)
            {
                above = pAbove1;
                left  = pLeft1;
            }
            primitives.intra_pred_planar((pixel*)above + 1, (pixel*)left + 1, pred, stride, width);
            modeCosts[PLANAR_IDX] = sa8d(fenc, stride, pred, stride);

            // Transpose NxN
            x265::primitives.transpose[nLog2SizeMinus2](buf_trans, (pixel*)fenc, stride);

            x265::primitives.intra_pred_allangs[nLog2SizeMinus2](tmp, pAbove0, pLeft0, pAbove1, pLeft1, (width <= 16));

            // TODO: We need SATD_x4 here
            for (UInt mode = 2; mode < numModesAvailable; mode++)
            {
                bool modeHor = (mode < 18);
                Pel *cmp = (modeHor ? buf_trans : fenc);
                intptr_t srcStride = (modeHor ? width : stride);
                modeCosts[mode] = sa8d(cmp, srcStride, &tmp[(mode - 2) * (width * width)], width);
            }
        }
        else
        {
            ALIGN_VAR_32(Pel, buf_scale[32 * 32]);
            x265::primitives.scale2D_64to32(buf_scale, fenc, stride);
            x265::primitives.transpose[3](buf_trans, buf_scale, 32);

            Pel above[2 * 32 + 1];
            Pel left[2 * 32 + 1];

            above[0] = left[0] = pAbove0[0];
            x265::primitives.scale1D_128to64(above + 1, pAbove0 + 1, 0);
            x265::primitives.scale1D_128to64(left + 1, pLeft0 + 1, 0);

            // 1
            primitives.intra_pred_dc(above + 1, left + 1, tmp, 32, 32, false);
            modeCosts[DC_IDX] = 4 * x265::primitives.sa8d[3](buf_scale, 32, tmp, 32);

            // 0
            primitives.intra_pred_planar((pixel*)above + 1, (pixel*)left + 1, tmp, 32, 32);
            modeCosts[PLANAR_IDX] = 4 * x265::primitives.sa8d[3](buf_scale, 32, tmp, 32);

            x265::primitives.intra_pred_allangs[3](tmp, above, left, above, left, false);

            // TODO: I use 4 of SATD32x32 to replace real 64x64
            for (UInt mode = 2; mode < numModesAvailable; mode++)
            {
                bool modeHor = (mode < 18);
                Pel *cmp_buf = (modeHor ? buf_trans : buf_scale);
                modeCosts[mode] = 4 * x265::primitives.sa8d[3]((pixel*)cmp_buf, 32, (pixel*)&tmp[(mode - 2) * (32 * 32)], 32);
            }
        }

        // SJB: TODO - all this code looks redundant. You just want the least cost of the 35

        // Find N least cost modes. N = numModesForFullRD
        for (UInt mode = 0; mode < numModesAvailable; mode++)
        {
            UInt sad = modeCosts[mode];
            UInt bits = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
            UInt64 cost = m_rdCost->calcRdSADCost(sad, bits);
            CandNum += m_search->xUpdateCandList(mode, cost, numModesForFullRD, rdModeList, CandCostList);
        }

        Int preds[3] = { -1, -1, -1 };
        Int mode = -1;
        Int numCand = cu->getIntraDirLumaPredictor(partOffset, preds, &mode);
        if (mode >= 0)
        {
            numCand = mode;
        }

        for (Int j = 0; j < numCand; j++)
        {
            Bool mostProbableModeIncluded = false;
            UInt mostProbableMode = preds[j];

            for (UInt i = 0; i < numModesForFullRD; i++)
            {
                mostProbableModeIncluded |= (mostProbableMode == rdModeList[i]);
            }

            if (!mostProbableModeIncluded)
            {
                rdModeList[numModesForFullRD++] = mostProbableMode;
            }
        }
    }

    // generate predYuv for the best mode
    cu->setLumaIntraDirSubParts(rdModeList[0], partOffset, depth + initTrDepth);

    // set context models
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
}

/** check RD costs for a CU block encoded with merge
 * \param outTempCU
 * \returns Void
 */

Void TEncCu::xComputeCostInter(TComDataCU* outTempCU, PartSize partSize, UInt index, Bool bUseMRG)
{
    UChar depth = outTempCU->getDepth(0);

    outTempCU->setDepthSubParts(depth, 0);
    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    outTempCU->setMergeAMP(true);
    m_tmpRecoYuv[depth]->clear();
    m_tmpResiYuv[depth]->clear();

    m_search->predInterSearch(outTempCU, m_origYuv[depth], m_modePredYuv[index][depth], bUseMRG);
    int part = PartitionFromSizes(outTempCU->getWidth(0), outTempCU->getHeight(0));
    outTempCU->m_totalCost = primitives.sse_pp[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                        m_modePredYuv[index][depth]->getLumaAddr(), m_modePredYuv[index][depth]->getStride());
}

Void TEncCu::xCompressInterCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU*& cu, UInt depth, UInt PartitionIndex)
{
#if CU_STAT_LOGFILE
    cntTotalCu[depth]++;
#endif
    m_abortFlag = false;

    TComPic* pic = outTempCU->getPic();

    // get Original YUV data from picture
    m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), outTempCU->getAddr(), outTempCU->getZorderIdxInCU());

    // variables for fast encoder decision
    Bool bTrySplit = true;
    Bool bSubBranch = true;
    Bool bTrySplitDQP = true;
    Bool bBoundary = false;

    UInt lpelx = outTempCU->getCUPelX();
    UInt rpelx = lpelx + outTempCU->getWidth(0)  - 1;
    UInt tpely = outTempCU->getCUPelY();
    UInt bpely = tpely + outTempCU->getHeight(0) - 1;

    Int qp = m_cfg->getUseRateCtrl() ? m_rateControl->getRCQP() : outTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * slice = outTempCU->getPic()->getSlice();
    Bool bSliceEnd = (slice->getSliceCurEndCUAddr() > outTempCU->getSCUAddr() && slice->getSliceCurEndCUAddr() < outTempCU->getSCUAddr() + outTempCU->getTotalNumPart());
    Bool bInsidePicture = (rpelx < outTempCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (bpely < outTempCU->getSlice()->getSPS()->getPicHeightInLumaSamples());

    // We need to split, so don't try these modes.
    TComYuv* tempYuv = NULL;
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit = true;

        /* Initialise all Mode-CUs based on parentCU */
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
            m_interCU_2Nx2N[depth]->initSubCU(cu, PartitionIndex, depth, qp);
            m_interCU_2NxN[depth]->initSubCU(cu, PartitionIndex, depth, qp);
            m_interCU_Nx2N[depth]->initSubCU(cu, PartitionIndex, depth, qp);
            m_intraInInterCU[depth]->initSubCU(cu, PartitionIndex, depth, qp);
            m_mergeCU[depth]->initSubCU(cu, PartitionIndex, depth, qp);
            m_bestMergeCU[depth]->initSubCU(cu, PartitionIndex, depth, qp);
        }

        /* Compute  Merge Cost */
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

            tempYuv = m_modePredYuv[0][depth];
            m_modePredYuv[0][depth] = m_bestPredYuv[depth];
            m_bestPredYuv[depth] = tempYuv;

            if (m_interCU_Nx2N[depth]->m_totalCost < outBestCU->m_totalCost)
            {
                outBestCU = m_interCU_Nx2N[depth];

                tempYuv = m_modePredYuv[1][depth];
                m_modePredYuv[1][depth] = m_bestPredYuv[depth];
                m_bestPredYuv[depth] = tempYuv;
            }
            if (m_interCU_2NxN[depth]->m_totalCost < outBestCU->m_totalCost)
            {
                outBestCU = m_interCU_2NxN[depth];

                tempYuv = m_modePredYuv[2][depth];
                m_modePredYuv[2][depth] = m_bestPredYuv[depth];
                m_bestPredYuv[depth] = tempYuv;
            }

            m_search->encodeResAndCalcRdInterCU(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth], m_bestResiYuv[depth], m_bestRecoYuv[depth], false);
#if CU_STAT_LOGFILE
            fprintf(fp1, "\n N : %d ,  Best Inter : %d , ", outBestCU->getWidth(0) / 2, outBestCU->m_totalCost);
#endif

            if (m_bestMergeCU[depth]->m_totalCost < outBestCU->m_totalCost)
            {
                outBestCU = m_bestMergeCU[depth];
                tempYuv = m_modePredYuv[3][depth];
                m_modePredYuv[3][depth] = m_bestPredYuv[depth];
                m_bestPredYuv[depth] = tempYuv;

                tempYuv = m_bestRecoYuv[depth];
                m_bestRecoYuv[depth] = m_bestMergeRecoYuv[depth];
                m_bestMergeRecoYuv[depth] = tempYuv;
            }

            /*compute intra cost */
            if (outBestCU->getCbf(0, TEXT_LUMA) != 0 ||
                outBestCU->getCbf(0, TEXT_CHROMA_U) != 0 ||
                outBestCU->getCbf(0, TEXT_CHROMA_V) != 0)
            {
                xComputeCostIntraInInter(m_intraInInterCU[depth], SIZE_2Nx2N);
                xEncodeIntraInInter(m_intraInInterCU[depth], m_origYuv[depth], m_modePredYuv[5][depth], m_tmpResiYuv[depth],  m_tmpRecoYuv[depth]);

                if (m_intraInInterCU[depth]->m_totalCost < outBestCU->m_totalCost)
                {
                    outBestCU = m_intraInInterCU[depth];
                    tempYuv = m_modePredYuv[5][depth];
                    m_modePredYuv[5][depth] = m_bestPredYuv[depth];
                    m_bestPredYuv[depth] = tempYuv;

                    TComYuv* tmpPic = m_bestRecoYuv[depth];
                    m_bestRecoYuv[depth] = m_tmpRecoYuv[depth];
                    m_tmpRecoYuv[depth] = tmpPic;
                }
            }
        }
        else
        {
            outBestCU = m_bestMergeCU[depth];
            tempYuv = m_modePredYuv[3][depth];
            m_modePredYuv[3][depth] = m_bestPredYuv[depth];
            m_bestPredYuv[depth] = tempYuv;

            tempYuv = m_bestRecoYuv[depth];
            m_bestRecoYuv[depth] = m_bestMergeRecoYuv[depth];
            m_bestMergeRecoYuv[depth] = tempYuv;
        }

        /* Disable recursive analysis for whole CUs temporarily */
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
        outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        outBestCU->m_totalBins += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        outBestCU->m_totalCost  = m_rdCost->calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }
#if CU_STAT_LOGFILE
    if (outBestCU)
    {
        fprintf(fp1, "Inter 2Nx2N_Merge : %d , Intra : %d",  m_bestMergeCU[depth]->m_totalCost, m_intraInInterCU[depth]->m_totalCost);
        if (outBestCU != m_bestMergeCU[depth] && outBestCU != m_intraInInterCU[depth])
            fprintf(fp1, " , Best  Part Mode chosen :%d, Pred Mode : %d",  outBestCU->getPartitionSize(0), outBestCU->getPredictionMode(0));
    }
#endif

    // further split
    if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
    {
        outTempCU->initEstData(depth, qp);
        UChar nextDepth = (UChar)(depth + 1);
        /*Best CU initialised to NULL; */
        TComDataCU* subBestPartCU = NULL;
        /*The temp structure is used for boundary analysis, and to copy Best SubCU mode data on return*/
        TComDataCU* subTempPartCU = m_tempCU[nextDepth];

        for (UInt partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
        {
            subBestPartCU = NULL;
            subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.

            Bool bInSlice = subTempPartCU->getSCUAddr() < slice->getSliceCurEndCUAddr();
            if (bInSlice && (subTempPartCU->getCUPelX() < slice->getSPS()->getPicWidthInLumaSamples()) &&
                (subTempPartCU->getCUPelY() < slice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[nextDepth][CI_NEXT_BEST]);
                }
                xCompressInterCU(subBestPartCU, subTempPartCU, outTempCU, nextDepth, partUnitIdx);

                /* Adding costs from best SUbCUs */
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth, true); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(subBestPartCU->getTotalNumPart() * partUnitIdx, nextDepth);
            }
            else if (bInSlice)
            {
                subTempPartCU->copyToPic((UChar)nextDepth);
                outTempCU->copyPartFrom(subTempPartCU, partUnitIdx, nextDepth, false);
            }
        }

        if (!bBoundary)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(outTempCU, 0, depth, true);

            outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
            outTempCU->m_totalBins += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        }

        outTempCU->m_totalCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        if ((g_maxCUWidth >> depth) == outTempCU->getSlice()->getPPS()->getMinCuDQPSize() && outTempCU->getSlice()->getPPS()->getUseDQP())
        {
            Bool hasResidual = false;
            for (UInt uiBlkIdx = 0; uiBlkIdx < outTempCU->getTotalNumPart(); uiBlkIdx++)
            {
                if (outTempCU->getCbf(uiBlkIdx, TEXT_LUMA) || outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_U) ||
                    outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_V))
                {
                    hasResidual = true;
                    break;
                }
            }

            UInt targetPartIdx = 0;
            if (hasResidual)
            {
                Bool foundNonZeroCbf = false;
                outTempCU->setQPSubCUs(outTempCU->getRefQP(targetPartIdx), outTempCU, 0, depth, foundNonZeroCbf);
                assert(foundNonZeroCbf);
            }
            else
            {
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
            }
        }

        m_rdSbacCoders[nextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

#if  CU_STAT_LOGFILE
        if (outBestCU != 0)
        {
            if (outBestCU->m_totalCost < outTempCU->m_totalCost)
            {
                fprintf(fp1, "\n%d vs %d  : selected mode :N : %d , cost : %d , Part mode : %d , Pred Mode : %d ", outBestCU->getWidth(0) / 2, outTempCU->getWidth(0) / 2, outBestCU->getWidth(0) / 2, outBestCU->m_totalCost,  outBestCU->getPartitionSize(0), outBestCU->getPredictionMode(0));
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
                fprintf(fp1, "\n  %d vs %d  : selected mode :N : %d , cost : %d  ", outBestCU->getWidth(0) / 2, outTempCU->getWidth(0) / 2, outTempCU->getWidth(0) / 2, outTempCU->m_totalCost);
                cntSplit[depth]++;
            }
        }
#endif // if  LOGGING

        /*If Best Mode is not NULL; then compare costs. Else assign best mode to Sub-CU costs
        Copy Recon data from Temp structure to Best structure*/
        if (outBestCU)
        {
            if (outTempCU->m_totalCost < outBestCU->m_totalCost)
            {
                outBestCU = outTempCU;
                tempYuv = m_tmpRecoYuv[depth];
                m_tmpRecoYuv[depth] = m_bestRecoYuv[depth];
                m_bestRecoYuv[depth] = tempYuv;
            }
        }
        else
        {
            outBestCU = outTempCU;
            tempYuv = m_tmpRecoYuv[depth];
            m_tmpRecoYuv[depth] = m_bestRecoYuv[depth];
            m_bestRecoYuv[depth] = tempYuv;
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

    /* Copy Best data to Picture for next partition prediction. */
    outBestCU->copyToPic((UChar)depth);

    /* Copy Yuv data to picture Yuv */
    xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, lpelx, tpely);

    if (bBoundary || (bSliceEnd && bInsidePicture))
    {
        return;
    }

    /* Assert if Best prediction mode is NONE
     Selected mode's RD-cost must be not MAX_DOUBLE.*/
    assert(outBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(outBestCU->getPredictionMode(0) != MODE_NONE);
    assert(outBestCU->m_totalCost != MAX_DOUBLE);
}
