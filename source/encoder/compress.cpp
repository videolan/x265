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

#if _MSC_VER
#pragma warning (disable: 4244)
#pragma warning (disable: 4018)
#endif

Void TEncCu::xCompressInterCU(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth)
{
    m_abortFlag = false;
    TComPic* pcPic = rpcBestCU->getPic();

    // get Original YUV data from picture
    m_ppcOrigYuv[uiDepth]->copyFromPicYuv(pcPic->getPicYuvOrg(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU());

    // variables for fast encoder decision
    Bool    bTrySplit    = true;

    // variable for Early CU determination
    Bool    bSubBranch = true;

    // variable for Cbf fast mode PU decision
    Bool doNotBlockPu = true;
    Bool earlyDetectionSkipMode = false;

    Bool bTrySplitDQP  = true;

    Bool bBoundary = false;
    UInt uiLPelX   = rpcBestCU->getCUPelX();
    UInt uiRPelX   = uiLPelX + rpcBestCU->getWidth(0)  - 1;
    UInt uiTPelY   = rpcBestCU->getCUPelY();
    UInt uiBPelY   = uiTPelY + rpcBestCU->getHeight(0) - 1;

    Int iQP = m_pcEncCfg->getUseRateCtrl() ? m_pcRateCtrl->getRCQP() : rpcTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * pcSlice = rpcTempCU->getPic()->getSlice(rpcTempCU->getPic()->getCurrSliceIdx());
    Bool bSliceEnd = (pcSlice->getSliceCurEndCUAddr() > rpcTempCU->getSCUAddr() && pcSlice->getSliceCurEndCUAddr() < rpcTempCU->getSCUAddr() + rpcTempCU->getTotalNumPart());
    Bool bInsidePicture = (uiRPelX < rpcBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < rpcBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples());
    // We need to split, so don't try these modes.
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit    = true;

        rpcTempCU->initEstData(uiDepth, iQP);

        // do inter modes, SKIP and 2Nx2N
        
        // 2Nx2N
        if (m_pcEncCfg->getUseEarlySkipDetection())
        {
            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N);
            rpcTempCU->initEstData(uiDepth, iQP);                              //by Competition for inter_2Nx2N
        }
        // SKIP
        xCheckRDCostMerge2Nx2N(rpcBestCU, rpcTempCU, &earlyDetectionSkipMode); //by Merge for inter_2Nx2N
        rpcTempCU->initEstData(uiDepth, iQP);

        if (!m_pcEncCfg->getUseEarlySkipDetection())
        {
            // 2Nx2N, NxN
            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N);
            rpcTempCU->initEstData(uiDepth, iQP);
            if (m_pcEncCfg->getUseCbfFastMode())
            {
                doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
            }
        }
        bTrySplitDQP = bTrySplit;

        if (uiDepth <= m_addSADDepth)
        {
            m_LCUPredictionSAD += m_temporalSAD;
            m_addSADDepth = uiDepth;
        }

        if (!earlyDetectionSkipMode)
        {
            rpcTempCU->initEstData(uiDepth, iQP);

            // do inter modes, NxN, 2NxN, and Nx2N
            
            // 2Nx2N, NxN
            if (!((rpcBestCU->getWidth(0) == 8) && (rpcBestCU->getHeight(0) == 8)))
            {
                if (uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth && doNotBlockPu)
                {
                    xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_NxN);
                    rpcTempCU->initEstData(uiDepth, iQP);
                }
            }

            if (m_pcEncCfg->getUseRectInter())
            {
                // 2NxN, Nx2N
                if (doNotBlockPu)
                {
                    xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_Nx2N);
                    rpcTempCU->initEstData(uiDepth, iQP);
                    if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_Nx2N)
                    {
                        doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                    }
                }
                if (doNotBlockPu)
                {
                    xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxN);
                    rpcTempCU->initEstData(uiDepth, iQP);
                    if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
                    {
                        doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                    }
                }
            }      

            // do normal intra modes
            // speedup for inter frames
            if (rpcBestCU->getCbf(0, TEXT_LUMA) != 0   ||
                rpcBestCU->getCbf(0, TEXT_CHROMA_U) != 0   ||
                rpcBestCU->getCbf(0, TEXT_CHROMA_V) != 0) // avoid very complex intra if it is unlikely
            {
                xCheckRDCostIntrainInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N);
                rpcTempCU->initEstData(uiDepth, iQP);

                if (uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth)
                {
                    if (rpcTempCU->getWidth(0) > (1 << rpcTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()))
                    {
                        xCheckRDCostIntrainInter(rpcBestCU, rpcTempCU, SIZE_NxN);
                        rpcTempCU->initEstData(uiDepth, iQP);
                    }
                }
            }            
        }

        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeSplitFlag(rpcBestCU, 0, uiDepth, true);
        rpcBestCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
        rpcBestCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        rpcBestCU->getTotalCost()  = CALCRDCOST(rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion(), m_pcRdCost->m_dLambda);

        // Early CU determination
        if (rpcBestCU->isSkipped(0))
        {
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

    rpcTempCU->initEstData(uiDepth, iQP);

    // further split
    if (bSubBranch && bTrySplitDQP && uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth)
    {
        UChar       uhNextDepth         = uiDepth + 1;
        TComDataCU* pcSubBestPartCU     = m_ppcBestCU[uhNextDepth];
        TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];
        UInt uiPartUnitIdx = 0;
        for (; uiPartUnitIdx < 4; uiPartUnitIdx++)
        {
            pcSubBestPartCU->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.
            pcSubTempPartCU->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.

            Bool bInSlice = pcSubBestPartCU->getSCUAddr() < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (pcSubBestPartCU->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (pcSubBestPartCU->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == uiPartUnitIdx) //initialize RD with previous depth buffer
                {
                    m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
                }
                else
                {
                    m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
                }

                // The following if condition has to be commented out in case the early Abort based on comparison of parentCu cost, childCU cost is not required.
                if (rpcBestCU->isIntra(0))
                {
                    xCompressInterCU(pcSubBestPartCU, pcSubTempPartCU, uhNextDepth);
                }
                else
                {
                    xCompressInterCU(pcSubBestPartCU, pcSubTempPartCU, uhNextDepth);
                }
                {
                    rpcTempCU->copyPartFrom(pcSubBestPartCU, uiPartUnitIdx, uhNextDepth); // Keep best part data to current temporary data.
                    xCopyYuv2Tmp(pcSubBestPartCU->getTotalNumPart() * uiPartUnitIdx, uhNextDepth);
                }
            }
            else if (bInSlice)
            {
                pcSubBestPartCU->copyToPic(uhNextDepth);
                rpcTempCU->copyPartFrom(pcSubBestPartCU, uiPartUnitIdx, uhNextDepth);
            }
        }

        if (!bBoundary)
        {
            m_pcEntropyCoder->resetBits();
            m_pcEntropyCoder->encodeSplitFlag(rpcTempCU, 0, uiDepth, true);

            rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits();     // split bits
            rpcTempCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        }
        rpcTempCU->getTotalCost()  = CALCRDCOST(rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion(), m_pcRdCost->m_dLambda);

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
#if !RDO_WITHOUT_DQP_BITS
                m_pcEntropyCoder->resetBits();
                m_pcEntropyCoder->encodeQP(rpcTempCU, uiTargetPartIdx, false);
                rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits();     // dQP bits
                rpcTempCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
                rpcTempCU->getTotalCost()  = CALCRDCOST(rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion(), m_pcRdCost->m_dLambda);
#endif

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
        xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth);                                 // RD compare current larger prediction
        // with sub partitioned prediction.
    }
    rpcBestCU->copyToPic(uiDepth);                                                   // Copy Best data to Picture for next partition prediction.
    xCopyYuv2Pic(rpcBestCU->getPic(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU(), uiDepth, uiDepth, rpcBestCU, uiLPelX, uiTPelY);   // Copy Yuv data to picture Yuv

    if (bBoundary || (bSliceEnd && bInsidePicture))
    {
        return;
    }

    // Assert if Best prediction mode is NONE
    // Selected mode's RD-cost must be not MAX_DOUBLE.
    assert(rpcBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(rpcBestCU->getPredictionMode(0) != MODE_NONE);
    assert(rpcBestCU->getTotalCost() != MAX_DOUBLE);
}

#if _MSC_VER
#pragma warning (default: 4244)
#pragma warning (disable: 4018)
#endif