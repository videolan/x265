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

/* Lambda Partition Select adjusts the threshold value for Early Exit in No-RDO flow */
#define LAMBDA_PARTITION_SELECT     0.9
#define EARLY_EXIT                  1
#define TOPSKIP                     1

using namespace x265;

void TEncCu::xEncodeIntraInInter(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv,  TShortYUV* outResiYuv, TComYuv* outReconYuv)
{
    uint64_t puCost = 0;
    uint32_t puDistY = 0;
    uint32_t puDistC = 0;
    uint32_t depth = cu->getDepth(0);
    uint32_t initTrDepth = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;

    // set context models
    m_search->m_rdGoOnSbacCoder->load(m_search->m_rdSbacCoders[depth][CI_CURR_BEST]);

    m_search->xRecurIntraCodingQT(cu, initTrDepth, 0, true, fencYuv, predYuv, outResiYuv, puDistY, puDistC, false, puCost);
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
    bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(cu, 0, depth, cu->getWidth(0), cu->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    cu->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();
    cu->m_totalCost = m_rdCost->calcRdCost(cu->m_totalDistortion, cu->m_totalBits);
    xCheckDQP(cu);
}

void TEncCu::xComputeCostIntraInInter(TComDataCU* cu, PartSize partSize)
{
    uint32_t depth = cu->getDepth(0);

    cu->setSkipFlagSubParts(false, 0, depth);
    cu->setPartSizeSubParts(partSize, 0, depth);
    cu->setPredModeSubParts(MODE_INTRA, 0, depth);
    cu->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);

    uint32_t initTrDepth = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    uint32_t width       = cu->getWidth(0) >> initTrDepth;
    uint32_t partOffset  = 0;

    //===== init pattern for luma prediction =====
    cu->getPattern()->initPattern(cu, initTrDepth, partOffset);
    // Reference sample smoothing
    cu->getPattern()->initAdiPattern(cu, partOffset, initTrDepth, m_search->getPredicBuf(), m_search->getPredicBufWidth(),
                                     m_search->getPredicBufHeight(), m_search->refAbove, m_search->refLeft,
                                     m_search->refAboveFlt, m_search->refLeftFlt);

    Pel* fenc   = m_origYuv[depth]->getLumaAddr(0, width);
    uint32_t stride = m_modePredYuv[5][depth]->getStride();

    Pel *above         = m_search->refAbove    + width - 1;
    Pel *aboveFiltered = m_search->refAboveFlt + width - 1;
    Pel *left          = m_search->refLeft     + width - 1;
    Pel *leftFiltered  = m_search->refLeftFlt  + width - 1;
    int sad, bsad;
    uint32_t bits, bbits, mode, bmode;
    uint64_t cost, bcost;

    // 33 Angle modes once
    ALIGN_VAR_32(Pel, buf_trans[32 * 32]);
    ALIGN_VAR_32(Pel, tmp[33 * 32 * 32]);
    int scaleWidth = width;
    int scaleStride = stride;
    int costMultiplier = 1;

    if (width > 32)
    {
        // origin is 64x64, we scale to 32x32 and setup required parameters
        ALIGN_VAR_32(Pel, bufScale[32 * 32]);
        primitives.scale2D_64to32(bufScale, fenc, stride);
        fenc = bufScale;

        // reserve space in case primitives need to store data in above
        // or left buffers
        Pel _above[4 * 32 + 1];
        Pel _left[4 * 32 + 1];
        Pel *aboveScale  = _above + 2 * 32;
        Pel *leftScale   = _left + 2 * 32;
        aboveScale[0] = leftScale[0] = above[0];
        primitives.scale1D_128to64(aboveScale + 1, above + 1, 0);
        primitives.scale1D_128to64(leftScale + 1, left + 1, 0);

        scaleWidth = 32;
        scaleStride = 32;
        costMultiplier = 4;

        // Filtered and Unfiltered refAbove and refLeft pointing to above and left.
        above         = aboveScale;
        left          = leftScale;
        aboveFiltered = aboveScale;
        leftFiltered  = leftScale;
    }

    int log2SizeMinus2 = g_convertToBit[scaleWidth];
    pixelcmp_t sa8d = primitives.sa8d[log2SizeMinus2];

    // DC
    primitives.intra_pred[log2SizeMinus2][DC_IDX](tmp, scaleStride, left, above, 0, (scaleWidth <= 16));
    bsad = costMultiplier * sa8d(fenc, scaleStride, tmp, scaleStride);
    bmode = mode = DC_IDX;
    bbits  = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
    bcost = m_rdCost->calcRdSADCost(bsad, bbits);

    Pel *abovePlanar   = above;
    Pel *leftPlanar    = left;

    if (width >= 8 && width <= 32)
    {
        abovePlanar = aboveFiltered;
        leftPlanar  = leftFiltered;
    }

    // PLANAR
    primitives.intra_pred[log2SizeMinus2][PLANAR_IDX](tmp, scaleStride, leftPlanar, abovePlanar, 0, 0);
    sad = costMultiplier * sa8d(fenc, scaleStride, tmp, scaleStride);
    mode = PLANAR_IDX;
    bits = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
    cost = m_rdCost->calcRdSADCost(sad, bits);
    COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);

    // Transpose NxN
    primitives.transpose[log2SizeMinus2](buf_trans, fenc, scaleStride);

    primitives.intra_pred_allangs[log2SizeMinus2](tmp, above, left, aboveFiltered, leftFiltered, (scaleWidth <= 16));

    for (mode = 2; mode < 35; mode++)
    {
        bool modeHor = (mode < 18);
        Pel *cmp = (modeHor ? buf_trans : fenc);
        intptr_t srcStride = (modeHor ? scaleWidth : scaleStride);
        sad  = costMultiplier * sa8d(cmp, srcStride, &tmp[(mode - 2) * (scaleWidth * scaleWidth)], scaleWidth);
        bits = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
        cost = m_rdCost->calcRdSADCost(sad, bits);
        COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);
    }

    cu->m_totalBits = bbits;
    cu->m_totalDistortion = bsad;
    cu->m_totalCost = bcost;

    // generate predYuv for the best mode
    cu->setLumaIntraDirSubParts(bmode, partOffset, depth + initTrDepth);

    // set context models
    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
}

/** check RD costs for a CU block encoded with merge */
void TEncCu::xComputeCostInter(TComDataCU* outTempCU, TComYuv* outPredYuv, PartSize partSize, bool bUseMRG)
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

    //do motion compensation only for Luma since luma cost alone is calculated
    outTempCU->m_totalBits = 0;
    m_search->predInterSearch(outTempCU, outPredYuv, bUseMRG, true, false);
    int part =  g_convertToBit[outTempCU->getWidth(0)];
    uint32_t distortion = primitives.sa8d[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                outPredYuv->getLumaAddr(), outPredYuv->getStride());
    outTempCU->m_totalDistortion = distortion;
    outTempCU->m_totalCost = m_rdCost->calcRdSADCost(distortion, outTempCU->m_totalBits);
}

void TEncCu::xComputeCostMerge2Nx2N(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComYuv*& bestPredYuv, TComYuv*& yuvReconBest)
{
    assert(outTempCU->getSlice()->getSliceType() != I_SLICE);
    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar interDirNeighbours[MRG_MAX_NUM_CANDS];
    int numValidMergeCand = 0;

    for (uint32_t i = 0; i < outTempCU->getSlice()->getMaxNumMergeCand(); ++i)
    {
        interDirNeighbours[i] = 0;
    }

    UChar depth = outTempCU->getDepth(0);
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);
    outTempCU->getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours, numValidMergeCand);
    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    outTempCU->setMergeFlagSubParts(true, 0, 0, depth);

    outBestCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
    outBestCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);
    outBestCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outBestCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);
    outBestCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    outBestCU->setMergeFlagSubParts(true, 0, 0, depth);

    int part = g_convertToBit[outTempCU->getWidth(0)];
    int bestMergeCand = 0;
    uint32_t bitsCand = 0;
    for (int mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
    {
        // set MC parameters, interprets depth relative to LCU level
        outTempCU->setMergeIndexSubParts(mergeCand, 0, 0, depth);
        outTempCU->setInterDirSubParts(interDirNeighbours[mergeCand], 0, 0, depth);
        outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[0 + 2 * mergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level
        outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[1 + 2 * mergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level

        // do MC only for Luma part
        m_search->motionCompensation(outTempCU, m_tmpPredYuv[depth], REF_PIC_LIST_X, -1, true, false);
        bitsCand = mergeCand + 1;
        if (mergeCand == (int)m_cfg->param.maxNumMergeCand - 1)
        {
            bitsCand--;
        }
        outTempCU->m_totalBits = bitsCand;
        outTempCU->m_totalDistortion = primitives.sa8d[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                             m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride());
        outTempCU->m_totalCost = m_rdCost->calcRdSADCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        if (outTempCU->m_totalCost < outBestCU->m_totalCost)
        {
            bestMergeCand = mergeCand;
            TComDataCU* tmp = outTempCU;
            outTempCU = outBestCU;
            outBestCU = tmp;
            // Change Prediction data
            TComYuv* yuv = bestPredYuv;
            bestPredYuv = m_tmpPredYuv[depth];
            m_tmpPredYuv[depth] = yuv;
        }
    }

    outTempCU->setMergeIndexSubParts(bestMergeCand, 0, 0, depth);
    outTempCU->setInterDirSubParts(interDirNeighbours[bestMergeCand], 0, 0, depth);
    outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[0 + 2 * bestMergeCand], SIZE_2Nx2N, 0, 0);
    outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[1 + 2 * bestMergeCand], SIZE_2Nx2N, 0, 0);
    outTempCU->m_totalBits = outBestCU->m_totalBits;
    outTempCU->m_totalDistortion = outBestCU->m_totalDistortion;
    if (m_cfg->param.rdLevel > 2)
    {
        //calculate the motion compensation for chroma for the best mode selected
        int numPart = outBestCU->getNumPartInter();
        for (int partIdx = 0; partIdx < numPart; partIdx++)
        {
            m_search->motionCompensation(outBestCU, bestPredYuv, REF_PIC_LIST_X, partIdx, false, true);
        }

        TComDataCU* tmp;
        TComYuv *yuv;

        //No-residue mode
        m_search->encodeResAndCalcRdInterCU(outTempCU, m_origYuv[depth], bestPredYuv, m_tmpResiYuv[depth], m_bestResiYuv[depth], m_tmpRecoYuv[depth], true);
        xCheckDQP(outTempCU);

        tmp = outTempCU;
        outTempCU = outBestCU;
        outBestCU = tmp;

        yuv = yuvReconBest;
        yuvReconBest = m_tmpRecoYuv[depth];
        m_tmpRecoYuv[depth] = yuv;

        //Encode with residue
        m_search->encodeResAndCalcRdInterCU(outTempCU, m_origYuv[depth], bestPredYuv, m_tmpResiYuv[depth], m_bestResiYuv[depth], m_tmpRecoYuv[depth], false);
        xCheckDQP(outTempCU);

        if (outTempCU->m_totalCost < outBestCU->m_totalCost)    //Choose best from no-residue mode and residue mode
        {
            tmp = outTempCU;
            outTempCU = outBestCU;
            outBestCU = tmp;

            yuv = yuvReconBest;
            yuvReconBest = m_tmpRecoYuv[depth];
            m_tmpRecoYuv[depth] = yuv;
        }
    }
    x265_emms();
}

void TEncCu::xCompressInterCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU*& cu, uint32_t depth, uint32_t PartitionIndex, UChar minDepth)
{
    m_abortFlag = false;

    TComPic* pic = outTempCU->getPic();

    // get Original YUV data from picture
    m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), outTempCU->getAddr(), outTempCU->getZorderIdxInCU());

    // variables for fast encoder decision
    bool bTrySplit = true;
    bool bSubBranch = true;
    bool bTrySplitDQP = true;
    bool bBoundary = false;
    uint32_t lpelx = outTempCU->getCUPelX();
    uint32_t rpelx = lpelx + outTempCU->getWidth(0)  - 1;
    uint32_t tpely = outTempCU->getCUPelY();
    uint32_t bpely = tpely + outTempCU->getHeight(0) - 1;
    TComDataCU* subTempPartCU, * subBestPartCU;
    int qp = outTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * slice = outTempCU->getPic()->getSlice();
    bool bSliceEnd = slice->getSliceCurEndCUAddr() > outTempCU->getSCUAddr() &&
        slice->getSliceCurEndCUAddr() < outTempCU->getSCUAddr() + outTempCU->getTotalNumPart();
    bool bInsidePicture = (rpelx < outTempCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) &&
        (bpely < outTempCU->getSlice()->getSPS()->getPicHeightInLumaSamples());

    // We need to split, so don't try these modes.
    TComYuv* tempYuv = NULL;
#if TOPSKIP
    TComDataCU* colocated0 = outTempCU->getCUColocated(REF_PIC_LIST_0);
    TComDataCU* colocated1 = outTempCU->getCUColocated(REF_PIC_LIST_1);
    char currentQP = outTempCU->getQP(0);
    char previousQP = colocated0->getQP(0);
    UChar delta = 0, minDepth0 = 4, minDepth1 = 4;
    if (depth == 0)
    {
        double sum0 = 0;
        double sum1 = 0;
        double avgDepth0 = 0;
        double avgDepth1 = 0;
        double avgDepth = 0;
        for (uint32_t i = 0; i < outTempCU->getTotalNumPart(); i = i + 4)
        {
            if (colocated0 && colocated0->getDepth(i) < minDepth0)
                minDepth0 = colocated0->getDepth(i);
            if (colocated1 && colocated1->getDepth(i) < minDepth1)
                minDepth1 = colocated1->getDepth(i);
            if (colocated0)
                sum0 += (colocated0->getDepth(i) * 4);
            if (colocated1)
                sum1 += (colocated1->getDepth(i) * 4);
        }

        avgDepth0 = sum0 / outTempCU->getTotalNumPart();
        avgDepth1 = sum1 / outTempCU->getTotalNumPart();
        avgDepth = (avgDepth0 + avgDepth1) / 2;

        if (minDepth1 < minDepth0)
            minDepth = minDepth1;
        else
            minDepth = minDepth0;

        if (((currentQP - previousQP) < 0) || (((currentQP - previousQP) >= 0) && ((avgDepth - minDepth) > 0.5)))
            delta = 0;
        else
            delta = 1;
        if (minDepth > 0)
            minDepth = minDepth - delta;
    }
#endif // if TOPSKIP
#if TOPSKIP
    if (!(depth < minDepth)) //topskip
#endif
    {
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
            xComputeCostMerge2Nx2N(m_bestMergeCU[depth], m_mergeCU[depth], m_modePredYuv[3][depth], m_bestMergeRecoYuv[depth]);
            TComYuv* bestMergePred;
            bestMergePred = m_modePredYuv[3][depth];
            if (!(m_cfg->param.bEnableEarlySkip && m_bestMergeCU[depth]->isSkipped(0)))
            {
                /*Compute 2Nx2N mode costs*/
                {
                    xComputeCostInter(m_interCU_2Nx2N[depth], m_modePredYuv[0][depth], SIZE_2Nx2N);
                    /*Choose best mode; initialise outBestCU to 2Nx2N*/
                    outBestCU = m_interCU_2Nx2N[depth];
                    tempYuv = m_modePredYuv[0][depth];
                    m_modePredYuv[0][depth] = m_bestPredYuv[depth];
                    m_bestPredYuv[depth] = tempYuv;
                }

                bTrySplitDQP = bTrySplit;

                if ((int)depth <= m_addSADDepth)
                {
                    m_LCUPredictionSAD += m_temporalSAD;
                    m_addSADDepth = depth;
                }

                /*Compute Rect costs*/
                if (m_cfg->param.bEnableRectInter)
                {
                    xComputeCostInter(m_interCU_Nx2N[depth], m_modePredYuv[1][depth], SIZE_Nx2N);
                    xComputeCostInter(m_interCU_2NxN[depth], m_modePredYuv[2][depth], SIZE_2NxN);
                }

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
                if (m_cfg->param.rdLevel > 2)
                {
                    //calculate the motion compensation for chroma for the best mode selected
                    int numPart = outBestCU->getNumPartInter();
                    for (int partIdx = 0; partIdx < numPart; partIdx++)
                    {
                        m_search->motionCompensation(outBestCU, m_bestPredYuv[depth], REF_PIC_LIST_X, partIdx, false, true);
                    }

                    m_search->encodeResAndCalcRdInterCU(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth],
                                                        m_bestResiYuv[depth], m_bestRecoYuv[depth], false);
                }
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

                /* Check for Intra in inter frames only if its a P-slice*/
                if (outBestCU->getSlice()->getSliceType() == P_SLICE)
                {
                    /*compute intra cost */
                    bool bdoIntra = true;
                    if (m_cfg->param.rdLevel > 2)
                    {
                        bdoIntra = (outBestCU->getCbf(0, TEXT_LUMA) ||  outBestCU->getCbf(0, TEXT_CHROMA_U) ||
                                    outBestCU->getCbf(0, TEXT_CHROMA_V));
                    }
                    else
                    {
                        uint32_t threshold[4] = { 20000, 6000, 1600, 500 };
                        int index = 4 - g_convertToBit[outBestCU->getWidth(0)];
                        bdoIntra = (outBestCU->m_totalDistortion > threshold[index]);
                    }
                    if (bdoIntra)
                    {
                        xComputeCostIntraInInter(m_intraInInterCU[depth], SIZE_2Nx2N);
                        if (m_cfg->param.rdLevel > 2)
                        {
                            xEncodeIntraInInter(m_intraInInterCU[depth], m_origYuv[depth], m_modePredYuv[5][depth],
                                                m_tmpResiYuv[depth],  m_tmpRecoYuv[depth]);
                        }
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
            if (m_cfg->param.rdLevel <= 2)
            {
                if (outBestCU->getPredictionMode(0) == MODE_INTER)
                {
                    int numPart = outBestCU->getNumPartInter();
                    for (int partIdx = 0; partIdx < numPart; partIdx++)
                    {
                        m_search->motionCompensation(outBestCU, m_bestPredYuv[depth], REF_PIC_LIST_X, partIdx, false, true);
                    }

                    if (m_cfg->param.rdLevel == 2)
                    {
                        m_search->encodeResAndCalcRdInterCU(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth],
                                                            m_bestResiYuv[depth], m_bestRecoYuv[depth], false);
                    }
                    else if (m_cfg->param.rdLevel <= 1)
                    {
                        m_tmpResiYuv[depth]->subtract(m_origYuv[depth], m_bestPredYuv[depth], 0, outBestCU->getWidth(0));
                        m_search->generateCoeffRecon(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth], m_bestRecoYuv[depth], false);
                    }
                }
                else
                {
                    if (m_cfg->param.rdLevel == 2)
                        xEncodeIntraInInter(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth],  m_bestRecoYuv[depth]);
                    else if (m_cfg->param.rdLevel == 1)
                    {
                        m_search->generateCoeffRecon(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth], m_bestRecoYuv[depth], false);
                    }
                }
                //Check Merge-skip
                if (m_cfg->param.rdLevel <= 2)
                {
                    if (!(outBestCU->getPredictionMode(0) == MODE_INTER && outBestCU->getPartitionSize(0) == SIZE_2Nx2N && outBestCU->getMergeFlag(0)))
                    {
                        int numPart = m_mergeCU[depth]->getNumPartInter();
                        for (int partIdx = 0; partIdx < numPart; partIdx++)
                        {
                            m_search->motionCompensation(m_mergeCU[depth], bestMergePred, REF_PIC_LIST_X, partIdx, false, true);
                        }
                    }

                    if (m_cfg->param.rdLevel == 2)
                    {
                        m_search->encodeResAndCalcRdInterCU(m_mergeCU[depth], m_origYuv[depth], bestMergePred, m_tmpResiYuv[depth],
                                                            m_bestResiYuv[depth], m_tmpRecoYuv[depth], true);

                        if (m_mergeCU[depth]->m_totalCost < outBestCU->m_totalCost)
                        {
                            outBestCU = m_mergeCU[depth];
                            tempYuv = m_bestRecoYuv[depth];
                            m_bestRecoYuv[depth] = m_tmpRecoYuv[depth];
                            m_tmpRecoYuv[depth] = tempYuv;
                            if (bestMergePred != m_bestPredYuv[depth])
                            {
                                bestMergePred->copyPartToPartYuv(m_bestPredYuv[depth], 0, outBestCU->getWidth(0), outBestCU->getHeight(0));
                            }
                        }
                    }
                    else if (m_cfg->param.rdLevel <= 1)
                    {
                        uint32_t threshold[4] = { 20000, 6000, 1600, 500 };
                        int index = 4 - g_convertToBit[outBestCU->getWidth(0)];
                        if (m_mergeCU[depth]->m_totalDistortion < threshold[index])
                        {
                            m_mergeCU[depth]->setSkipFlagSubParts(true, 0, depth);
                            m_search->generateCoeffRecon(m_mergeCU[depth], m_origYuv[depth], bestMergePred, m_tmpResiYuv[depth], m_bestRecoYuv[depth], true);
                            outBestCU = m_mergeCU[depth];
                        }
                    }
                }
            }
            
            xCheckDQP(outBestCU);
            /* Disable recursive analysis for whole CUs temporarily */
            if ((outBestCU != 0) && (outBestCU->isSkipped(0)))
                bSubBranch = false;
            else
                bSubBranch = true;

            if (m_cfg->param.rdLevel > 1)
            {
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth, true);
                outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
                outBestCU->m_totalCost  = m_rdCost->calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);
            }
        }
        else if (!(bSliceEnd && bInsidePicture))
        {
            bBoundary = true;
            m_addSADDepth++;
        }
    }

    // further split
    if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
    {
#if EARLY_EXIT // turn ON this to enable early exit
        // early exit when the RD cost of best mode at depth n is less than the sum of avgerage of RD cost of the neighbour
        // CU's(above, aboveleft, aboveright, left, colocated) and avg cost of that CU at depth "n"  with weightage for each quantity
#if TOPSKIP
        if (outBestCU != 0 && !(depth < minDepth)) //topskip
#else
        if (outBestCU != 0)
#endif
        {
            uint64_t totalCostNeigh = 0, totalCostCU = 0, totalCountCU = 0;
            double avgCost = 0;
            uint64_t totalCountNeigh = 0;
            TComDataCU* above = outTempCU->getCUAbove();
            TComDataCU* aboveLeft = outTempCU->getCUAboveLeft();
            TComDataCU* aboveRight = outTempCU->getCUAboveRight();
            TComDataCU* left = outTempCU->getCULeft();
            TComDataCU* rootCU = outTempCU->getPic()->getPicSym()->getCU(outTempCU->getAddr());

            totalCostCU += rootCU->m_avgCost[depth] * rootCU->m_count[depth];
            totalCountCU += rootCU->m_count[depth];
            if (above)
            {
                totalCostNeigh += above->m_avgCost[depth] * above->m_count[depth];
                totalCountNeigh += above->m_count[depth];
            }
            if (aboveLeft)
            {
                totalCostNeigh += aboveLeft->m_avgCost[depth] * aboveLeft->m_count[depth];
                totalCountNeigh += aboveLeft->m_count[depth];
            }
            if (aboveRight)
            {
                totalCostNeigh += aboveRight->m_avgCost[depth] * aboveRight->m_count[depth];
                totalCountNeigh += aboveRight->m_count[depth];
            }
            if (left)
            {
                totalCostNeigh += left->m_avgCost[depth] * left->m_count[depth];
                totalCountNeigh += left->m_count[depth];
            }

            //giving 60% weight to all CU's and 40% weight to neighbour CU's
            if (totalCountNeigh + totalCountCU)
                avgCost = ((0.6 * totalCostCU) + (0.4 * totalCostNeigh)) / ((0.6 * totalCountCU) + (0.4 * totalCountNeigh));

            float lambda = 1.0f;

            if (outBestCU->m_totalCost < lambda * avgCost && avgCost != 0 && depth != 0)
            {
                /* Copy Best data to Picture for next partition prediction. */
                outBestCU->copyToPic((UChar)depth);

                /* Copy Yuv data to picture Yuv */
                xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, lpelx, tpely);
                return;
            }
        }
#endif // if EARLY_EXIT
        outTempCU->initEstData(depth, qp);
        UChar nextDepth = (UChar)(depth + 1);
        subTempPartCU = m_tempCU[nextDepth];
        for (uint32_t nextDepth_partIndex = 0; nextDepth_partIndex < 4; nextDepth_partIndex++)
        {
            subBestPartCU = NULL;
            subTempPartCU->initSubCU(outTempCU, nextDepth_partIndex, nextDepth, qp); // clear sub partition datas or init.

            bool bInSlice = subTempPartCU->getSCUAddr() < slice->getSliceCurEndCUAddr();
            if (bInSlice && (subTempPartCU->getCUPelX() < slice->getSPS()->getPicWidthInLumaSamples()) &&
                (subTempPartCU->getCUPelY() < slice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == nextDepth_partIndex) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[nextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[nextDepth][CI_NEXT_BEST]);
                }
                xCompressInterCU(subBestPartCU, subTempPartCU, outTempCU, nextDepth, nextDepth_partIndex, minDepth);
#if EARLY_EXIT
                if (subBestPartCU->getPredictionMode(0) != MODE_INTRA)
                {
                    uint64_t tempavgCost = subBestPartCU->m_totalCost;
                    TComDataCU* rootCU = outTempCU->getPic()->getPicSym()->getCU(outTempCU->getAddr());
                    uint64_t temp = rootCU->m_avgCost[depth + 1] * rootCU->m_count[depth + 1];
                    rootCU->m_count[depth + 1] += 1;
                    rootCU->m_avgCost[depth + 1] = (temp + tempavgCost) / rootCU->m_count[depth + 1];
                }
#endif // if EARLY_EXIT
                /* Adding costs from best SUbCUs */
                outTempCU->copyPartFrom(subBestPartCU, nextDepth_partIndex, nextDepth, true); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(subBestPartCU->getTotalNumPart() * nextDepth_partIndex, nextDepth);
            }
            else if (bInSlice)
            {
                subTempPartCU->copyToPic((UChar)nextDepth);
                outTempCU->copyPartFrom(subTempPartCU, nextDepth_partIndex, nextDepth, false);
            }
        }

        if (!bBoundary)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(outTempCU, 0, depth, true);

            outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        }

        outTempCU->m_totalCost = m_rdCost->calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        if ((g_maxCUWidth >> depth) == outTempCU->getSlice()->getPPS()->getMinCuDQPSize() && outTempCU->getSlice()->getPPS()->getUseDQP())
        {
            bool hasResidual = false;
            for (uint32_t uiBlkIdx = 0; uiBlkIdx < outTempCU->getTotalNumPart(); uiBlkIdx++)
            {
                if (outTempCU->getCbf(uiBlkIdx, TEXT_LUMA) || outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_U) ||
                    outTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_V))
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
                assert(foundNonZeroCbf);
            }
            else
            {
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
            }
        }

        m_rdSbacCoders[nextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

        /* If Best Mode is not NULL; then compare costs. Else assign best mode to Sub-CU costs
         * Copy recon data from Temp structure to Best structure */
        if (outBestCU)
        {
#if EARLY_EXIT
            if (depth == 0)
            {
                uint64_t tempavgCost = outBestCU->m_totalCost;
                TComDataCU* rootCU = outTempCU->getPic()->getPicSym()->getCU(outTempCU->getAddr());
                uint64_t temp = rootCU->m_avgCost[depth] * rootCU->m_count[depth];
                rootCU->m_count[depth] += 1;
                rootCU->m_avgCost[depth] = (temp + tempavgCost) / rootCU->m_count[depth];
            }
#endif
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


    /* Copy Best data to Picture for next partition prediction. */
    outBestCU->copyToPic((UChar)depth);

    /* Copy Yuv data to picture Yuv */
    xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, lpelx, tpely);

    if (bBoundary || (bSliceEnd && bInsidePicture)) return;

    /* Assert if Best prediction mode is NONE
     Selected mode's RD-cost must be not MAX_DOUBLE.*/
    assert(outBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(outBestCU->getPredictionMode(0) != MODE_NONE);
    assert(outBestCU->m_totalCost != MAX_DOUBLE);
}
