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

using namespace x265;

#if CU_STAT_LOGFILE
extern int totalCU;
extern int cntInter[4], cntIntra[4], cntSplit[4], cntIntraNxN;
extern int cuInterDistribution[4][4], cuIntraDistribution[4][3];
extern int cntSkipCu[4],  cntTotalCu[4];
extern FILE* fp1;
#endif

void TEncCu::xEncodeIntraInInter(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv,  TShortYUV* outResiYuv, TComYuv* outReconYuv)
{
    UInt64 puCost = 0;
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
    cu->getPattern()->initAdiPattern(cu, partOffset, initTrDepth, m_search->getPredicBuf(),  m_search->getPredicBufWidth(),  m_search->getPredicBufHeight(), m_search->refAbove, m_search->refLeft, m_search->refAboveFlt, m_search->refLeftFlt);

    Pel* fenc   = m_origYuv[depth]->getLumaAddr(0, width);
    Pel* pred   = m_modePredYuv[5][depth]->getLumaAddr(0, width);
    uint32_t stride = m_modePredYuv[5][depth]->getStride();

    Pel *pAbove0 = m_search->refAbove    + width - 1;
    Pel *pAbove1 = m_search->refAboveFlt + width - 1;
    Pel *pLeft0  = m_search->refLeft     + width - 1;
    Pel *pLeft1  = m_search->refLeftFlt  + width - 1;
    int sad;
    uint32_t bits, mode, bmode;
    UInt64 cost, bcost;

    // 33 Angle modes once
    ALIGN_VAR_32(Pel, tmp[33 * 32 * 32]);
    ALIGN_VAR_32(Pel, buf_trans[32 * 32]);

    if (width < 64)
    {
        int nLog2SizeMinus2 = g_convertToBit[width];
        pixelcmp_t sa8d = primitives.sa8d[nLog2SizeMinus2];

        primitives.intra_pred_dc(pAbove0 + 1, pLeft0 + 1, pred, stride, width, (width <= 16));
        sad = sa8d(fenc, stride, pred, stride);
        bmode = mode = DC_IDX;
        bits = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
        bcost = m_rdCost->calcRdSADCost(sad, bits);

        Pel *above = pAbove0;
        Pel *left  = pLeft0;
        if (width >= 8)
        {
            above = pAbove1;
            left  = pLeft1;
        }
        primitives.intra_pred_planar(above + 1, left + 1, pred, stride, width);
        sad = sa8d(fenc, stride, pred, stride);
        mode = PLANAR_IDX;
        bits = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
        cost = m_rdCost->calcRdSADCost(sad, bits);
        COPY2_IF_LT(bcost, cost, bmode, mode);

        primitives.transpose[nLog2SizeMinus2](buf_trans, fenc, stride);
        primitives.intra_pred_allangs[nLog2SizeMinus2](tmp, pAbove0, pLeft0, pAbove1, pLeft1, (width <= 16));
        for (mode = 2; mode < 35; mode++)
        {
            bool modeHor = (mode < 18);
            Pel *cmp = (modeHor ? buf_trans : fenc);
            intptr_t srcStride = (modeHor ? width : stride);
            sad = sa8d(cmp, srcStride, &tmp[(mode - 2) * (width * width)], width);
            bits = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
            cost = m_rdCost->calcRdSADCost(sad, bits);
            COPY2_IF_LT(bcost, cost, bmode, mode);
        }
    }
    else
    {
        /* 64x64 intra doesn't really exist, so downscale to 32x32 and estimate */
        ALIGN_VAR_32(Pel, buf_scale[32 * 32]);
        pixelcmp_t sa8d = primitives.sa8d[BLOCK_32x32];
        primitives.scale2D_64to32(buf_scale, fenc, stride);
        width = 32;

        // some versions of intra angs primitives may write into above
        // and/or left buffers above the original pointer, so we must
        // reserve space for it
        Pel _above[4 * 32 + 1];
        Pel _left[4 * 32 + 1];
        Pel *const above = _above + 2 * 32;
        Pel *const left = _left + 2 * 32;

        above[0] = left[0] = pAbove0[0];
        primitives.scale1D_128to64(above + 1, pAbove0 + 1, 0);
        primitives.scale1D_128to64(left + 1, pLeft0 + 1, 0);

        primitives.intra_pred_dc(above + 1, left + 1, tmp, width, width, false);
        sad = 4 * sa8d(buf_scale, width, tmp, width);
        bmode = mode = DC_IDX;
        bits = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
        bcost = m_rdCost->calcRdSADCost(sad, bits);

        primitives.intra_pred_planar(above + 1, left + 1, tmp, width, width);
        sad = 4 * sa8d(buf_scale, width, tmp, width);
        mode = PLANAR_IDX;
        bits = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
        cost = m_rdCost->calcRdSADCost(sad, bits);
        COPY2_IF_LT(bcost, cost, bmode, mode);

        primitives.transpose[BLOCK_32x32](buf_trans, buf_scale, width);
        primitives.intra_pred_allangs[BLOCK_32x32](tmp, above, left, above, left, false);
        for (mode = 2; mode < 35; mode++)
        {
            Pel *cmp_buf = (mode < 18) ? buf_trans : buf_scale;
            sad = 4 * sa8d(cmp_buf, width, tmp + (mode - 2) * (32 * 32), width);
            bits = m_search->xModeBitsIntra(cu, mode, partOffset, depth, initTrDepth);
            cost = m_rdCost->calcRdSADCost(sad, bits);
            COPY2_IF_LT(bcost, cost, bmode, mode);
        }
    }

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
    int part = partitionFromSizes(outTempCU->getWidth(0), outTempCU->getHeight(0));
    uint32_t distortion = primitives.sse_pp[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                  outPredYuv->getLumaAddr(), outPredYuv->getStride());
    outTempCU->m_totalCost = m_rdCost->calcRdCost(distortion, outTempCU->m_totalBits);
}

void TEncCu::xComputeCostMerge2Nx2N(TComDataCU*& outBestCU, TComDataCU*& outTempCU, bool* earlyDetectionSkip, TComYuv*& bestPredYuv, TComYuv*& yuvReconBest)
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

    int bestMergeCand = 0;
    for (int mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand)
    {
        // set MC parameters, interprets depth relative to LCU level
        outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
        outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);
        outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
        outTempCU->setMergeFlagSubParts(true, 0, 0, depth);
        outTempCU->setMergeIndexSubParts(mergeCand, 0, 0, depth);
        outTempCU->setInterDirSubParts(interDirNeighbours[mergeCand], 0, 0, depth);
        outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[0 + 2 * mergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level
        outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[1 + 2 * mergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level

        // do MC only for Luma part
        m_search->motionCompensation(outTempCU, m_tmpPredYuv[depth], REF_PIC_LIST_X, -1, true, false);
        int part = partitionFromSizes(outTempCU->getWidth(0), outTempCU->getHeight(0));

        outTempCU->m_totalCost = primitives.sse_pp[part](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                         m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride());

        int orgQP = outTempCU->getQP(0);

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

        outTempCU->initEstData(depth, orgQP);
    }

    //calculate the motion compensation for chroma for the best mode selected
    int numPart = outBestCU->getNumPartInter();
    for (int partIdx = 0; partIdx < numPart; partIdx++)
    {
        m_search->motionCompensation(outBestCU, bestPredYuv, REF_PIC_LIST_X, partIdx, false, true);
    }

    TComDataCU* tmp;
    TComYuv *yuv;

    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(m_cfg->getCUTransquantBypassFlagValue(), 0, depth);
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    outTempCU->setMergeFlagSubParts(true, 0, 0, depth);
    outTempCU->setMergeIndexSubParts(bestMergeCand, 0, 0, depth);
    outTempCU->setInterDirSubParts(interDirNeighbours[bestMergeCand], 0, 0, depth);
    outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[0 + 2 * bestMergeCand], SIZE_2Nx2N, 0, 0);
    outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[1 + 2 * bestMergeCand], SIZE_2Nx2N, 0, 0);

    //No-residue mode
    m_search->encodeResAndCalcRdInterCU(outTempCU, m_origYuv[depth], bestPredYuv, m_tmpResiYuv[depth], m_bestResiYuv[depth], m_tmpRecoYuv[depth], true);

    tmp = outTempCU;
    outTempCU = outBestCU;
    outBestCU = tmp;

    yuv = yuvReconBest;
    yuvReconBest = m_tmpRecoYuv[depth];
    m_tmpRecoYuv[depth] = yuv;

    //Encode with residue
    m_search->encodeResAndCalcRdInterCU(outTempCU, m_origYuv[depth], bestPredYuv, m_tmpResiYuv[depth], m_bestResiYuv[depth], m_tmpRecoYuv[depth], false);

    if (outTempCU->m_totalCost < outBestCU->m_totalCost)    //Choose best from no-residue mode and residue mode
    {
        tmp = outTempCU;
        outTempCU = outBestCU;
        outBestCU = tmp;

        yuv = yuvReconBest;
        yuvReconBest = m_tmpRecoYuv[depth];
        m_tmpRecoYuv[depth] = yuv;
    }

    if (m_cfg->param.bEnableEarlySkip)
    {
        if (outBestCU->getQtRootCbf(0) == 0)
        {
            if (outBestCU->getMergeFlag(0))
            {
                *earlyDetectionSkip = true;
            }
            else
            {
                bool allZero = true;
                for (uint32_t list = 0; list < 2; list++)
                {
                    if (outBestCU->getSlice()->getNumRefIdx(list) > 0)
                    {
                        allZero &= !outBestCU->getCUMvField(list)->getMvd(0).word;
                    }
                }

                if (allZero)
                {
                    *earlyDetectionSkip = true;
                }
            }
        }
    }

    m_tmpResiYuv[depth]->clear();
    x265_emms();
}

void TEncCu::xCompressInterCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU*& cu, uint32_t depth, uint32_t PartitionIndex)
{
#if CU_STAT_LOGFILE
    cntTotalCu[depth]++;
#endif
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
        bool earlyDetectionSkip = false;
        xComputeCostMerge2Nx2N(m_bestMergeCU[depth], m_mergeCU[depth], &earlyDetectionSkip, m_modePredYuv[3][depth], m_bestMergeRecoYuv[depth]);

        if (!earlyDetectionSkip)
        {
            /*Compute 2Nx2N mode costs*/
            //if (depth == 0)
            {
                xComputeCostInter(m_interCU_2Nx2N[depth], m_modePredYuv[0][depth], SIZE_2Nx2N);
                /*Choose best mode; initialise outBestCU to 2Nx2N*/
                outBestCU = m_interCU_2Nx2N[depth];
                tempYuv = m_modePredYuv[0][depth];
                m_modePredYuv[0][depth] = m_bestPredYuv[depth];
                m_bestPredYuv[depth] = tempYuv;
            }
            //reusing the buffer gives md5 hash error - need to look into it. suspect it happens when inSlice condition is false.

            /*else
            {
                outBestCU = m_interCU_NxN[PartitionIndex][depth];
                tempYuv = m_bestPredYuvNxN[PartitionIndex][depth];
                m_bestPredYuvNxN[PartitionIndex][depth] = m_bestPredYuv[depth];
                m_bestPredYuv[depth] = tempYuv;
            }*/

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
            //calculate the motion compensation for chroma for the best mode selected
            int numPart = outBestCU->getNumPartInter();
            for (int partIdx = 0; partIdx < numPart; partIdx++)
            {
                m_search->motionCompensation(outBestCU, m_bestPredYuv[depth], REF_PIC_LIST_X, partIdx, false, true);
            }

            m_search->encodeResAndCalcRdInterCU(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth],
                                                m_bestResiYuv[depth], m_bestRecoYuv[depth], false);
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

            /* Check for Intra in inter frames only if its a P-slice*/
            if (outBestCU->getSlice()->getSliceType() == P_SLICE)
            {
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
#if EARLY_EXIT // turn ON this to enable early exit
        // early exit when the RD cost of best mode at depth n is less than the avgerage of RD cost of the
        // CU's(above, aboveleft, aboveright, left, colocated) at depth "n" of previosuly coded CU's
        if (outBestCU != 0)
        {
            UInt64 costCU = 0, costCUAbove = 0, costCUAboveLeft = 0, costCUAboveRight = 0, costCULeft = 0, costCUColocated0 = 0, costCUColocated1 = 0, totalCost = 0, avgCost= 0;
            UInt64 countCU = 0, countCUAbove = 0, countCUAboveLeft = 0, countCUAboveRight = 0, countCULeft = 0, countCUColocated0 = 0, countCUColocated1 = 0;
            UInt64 totalCount = 0;
            TComDataCU* above = outTempCU->getCUAbove();
            TComDataCU* aboveLeft = outTempCU->getCUAboveLeft();
            TComDataCU* aboveRight = outTempCU->getCUAboveRight();
            TComDataCU* left = outTempCU->getCULeft();
            TComDataCU* colocated0 = outTempCU->getCUColocated(REF_PIC_LIST_0);
            TComDataCU* colocated1 = outTempCU->getCUColocated(REF_PIC_LIST_1);

            costCU = outTempCU->m_avgCost[depth] * outTempCU->m_count[depth];
            countCU = outTempCU->m_count[depth];
            if (above)
            {
                costCUAbove = above->m_avgCost[depth] * above->m_count[depth];
                countCUAbove = above->m_count[depth];
            }
            if (aboveLeft)
            {
                costCUAboveLeft = aboveLeft->m_avgCost[depth] * aboveLeft->m_count[depth];
                countCUAboveLeft = aboveLeft->m_count[depth];
            }
            if (aboveRight)
            {
                costCUAboveRight = aboveRight->m_avgCost[depth] * aboveRight->m_count[depth];
                countCUAboveRight = aboveRight->m_count[depth];
            }
            if (left)
            {
                costCULeft = left->m_avgCost[depth] * left->m_count[depth];
                countCULeft = left->m_count[depth];
            }
            if (colocated0)
            {
                costCUColocated0 = colocated0->m_avgCost[depth] * colocated0->m_count[depth];
                countCUColocated0 = colocated0->m_count[depth];
            }
            if (colocated1)
            {
                costCUColocated1 = colocated1->m_avgCost[depth] * colocated1->m_count[depth];
                countCUColocated1 = colocated1->m_count[depth];
            }

            totalCost = costCU + costCUAbove + costCUAboveLeft + costCUAboveRight + costCULeft + costCUColocated0 + costCUColocated1;
            totalCount = countCU + countCUAbove + countCUAboveLeft + countCUAboveRight + countCULeft + countCUColocated0 + countCUColocated1;
            if (totalCount != 0)
                avgCost = totalCost / totalCount;

            float lambda = 1.0f;

            if (outBestCU->m_totalCost < lambda * avgCost && avgCost != 0 && depth != 0)
            {
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth, true);
                outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits();        // split bits
                outBestCU->m_totalCost  = m_rdCost->calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);
                /* Copy Best data to Picture for next partition prediction. */
                outBestCU->copyToPic((UChar)depth);

                /* Copy Yuv data to picture Yuv */
                xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, lpelx, tpely);
                return;
            }
        }
#endif
#if 0 // turn ON this to enable early exit
        //early exit when RD cost of best mode is less than the cumulative RD cost of 4 subpartition
        UInt64 nxnCost = 0;
        if (outBestCU != 0 && depth > 0)
        {
            outTempCU->initEstData(depth, qp);
            UChar nextDepth = (UChar)(depth + 1);
            /*Best CU initialised to NULL; */
            subBestPartCU = NULL;
            /*The temp structure is used for boundary analysis, and to copy Best SubCU mode data on return*/
            nxnCost = 0;
            for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
            {
                subTempPartCU = m_interCU_NxN[partUnitIdx][nextDepth];
                subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp);     // clear sub partition datas or init.
                TComPic* subPic = subTempPartCU->getPic();
                m_origYuv[nextDepth]->copyFromPicYuv(subPic->getPicYuvOrg(), subTempPartCU->getAddr(), subTempPartCU->getZorderIdxInCU());
                TComSlice * pcSubSlice = subTempPartCU->getPic()->getSlice();
                bool subSliceEnd = subTempPartCU->getSCUAddr() < slice->getSliceCurEndCUAddr()
                    && pcSubSlice->getSliceCurEndCUAddr() < subTempPartCU->getSCUAddr() + subTempPartCU->getTotalNumPart();
                if (!subSliceEnd && (subTempPartCU->getCUPelX() < slice->getSPS()->getPicWidthInLumaSamples()) &&
                    (subTempPartCU->getCUPelY() < slice->getSPS()->getPicHeightInLumaSamples()))
                {
                    if (outBestCU->getPredictionMode(0) == 0)
                    {
                        xComputeCostInter(subTempPartCU, m_bestPredYuvNxN[partUnitIdx][nextDepth], outBestCU->getPartitionSize(0), 0);
                        m_search->encodeResAndCalcRdInterCU(subTempPartCU, m_origYuv[nextDepth], m_bestPredYuvNxN[partUnitIdx][nextDepth], m_tmpResiYuv[nextDepth],
                                                            m_bestResiYuv[nextDepth], m_tmpRecoYuv[nextDepth], false);
                        nxnCost += subTempPartCU->m_totalCost;
                    }
                    else if (outBestCU->getPredictionMode(0) == 1)
                    {
                        xComputeCostIntraInInter(subTempPartCU, SIZE_2Nx2N);
                        xEncodeIntraInInter(subTempPartCU, m_origYuv[nextDepth], m_modePredYuv[5][nextDepth], m_tmpResiYuv[nextDepth],  m_tmpRecoYuv[nextDepth]);
                        nxnCost += subTempPartCU->m_totalCost;
                    }
                }
                subTempPartCU->copyToPic((UChar)nextDepth);
            }

            float lambda = 1.0f;
            if (outBestCU->getSlice()->getSliceType() == P_SLICE)
                lambda = 0.9f;
            else if (outBestCU->getSlice()->getSliceType() == B_SLICE)
                lambda = 1.1f;

            if (outBestCU->m_totalCost < lambda * nxnCost)
            {
                m_entropyCoder->resetBits();
                m_entropyCoder->encodeSplitFlag(outBestCU, 0, depth, true);
                outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits();        // split bits
                outBestCU->m_totalCost  = m_rdCost->calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);
                /* Copy Best data to Picture for next partition prediction. */
                outBestCU->copyToPic((UChar)depth);

                /* Copy Yuv data to picture Yuv */
                xCopyYuv2Pic(outBestCU->getPic(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU(), depth, depth, outBestCU, lpelx, tpely);
                return;
            }
        }
#endif // early exit
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
                xCompressInterCU(subBestPartCU, subTempPartCU, outTempCU, nextDepth, nextDepth_partIndex);
#if EARLY_EXIT
                for (int k = 0; k < 4; k++)
                {
                    outTempCU->m_avgCost[k] = subTempPartCU->m_avgCost[k];
                    outTempCU->m_count[k] = subTempPartCU->m_count[k];
                }
                if (subBestPartCU->getPredictionMode(0) != MODE_INTRA)
                {
                    UInt64 tempavgCost = subBestPartCU->m_totalCost;
                    UInt64 temp = outTempCU->m_avgCost[depth + 1] * outTempCU->m_count[depth + 1];
                    outTempCU->m_count[depth + 1] += 1;
                    outTempCU->getPic()->getPicSym()->getCU(outTempCU->getAddr())->m_count[depth + 1] += 1;
                    outTempCU->m_avgCost[depth + 1] = (temp + tempavgCost) / outTempCU->m_count[depth + 1];
                    outTempCU->getPic()->getPicSym()->getCU(outTempCU->getAddr())->m_avgCost[depth + 1] = outTempCU->m_avgCost[depth + 1];
                }
#endif
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

#if  CU_STAT_LOGFILE
        if (outBestCU != 0)
        {
            if (outBestCU->m_totalCost < outTempCU->m_totalCost)
            {
                fprintf(fp1, "\n%d vs %d  : selected mode :N : %d , cost : %d , Part mode : %d , Pred Mode : %d ",
                        outBestCU->getWidth(0) / 2, outTempCU->getWidth(0) / 2, outBestCU->getWidth(0) / 2, outBestCU->m_totalCost,
                        outBestCU->getPartitionSize(0), outBestCU->getPredictionMode(0));
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
                fprintf(fp1, "\n  %d vs %d  : selected mode :N : %d , cost : %d  ",
                        outBestCU->getWidth(0) / 2, outTempCU->getWidth(0) / 2, outTempCU->getWidth(0) / 2, outTempCU->m_totalCost);
                cntSplit[depth]++;
            }
        }
#endif // if  LOGGING

        /* If Best Mode is not NULL; then compare costs. Else assign best mode to Sub-CU costs
         * Copy recon data from Temp structure to Best structure */
        if (outBestCU)
        {
            if (depth == 0)
            {
                UInt64 tempavgCost = outBestCU->m_totalCost;
                UInt64 temp = outTempCU->m_avgCost[depth] * outTempCU->m_count[depth];
                outTempCU->m_count[depth] += 1;
                outTempCU->getPic()->getPicSym()->getCU(outTempCU->getAddr())->m_count[depth] += 1;

                outTempCU->m_avgCost[depth] = (temp + tempavgCost) / outTempCU->m_count[depth];
                outTempCU->getPic()->getPicSym()->getCU(outTempCU->getAddr())->m_avgCost[depth] = outTempCU->m_avgCost[depth];
            }
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
