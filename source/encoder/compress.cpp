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
    TComSlice * pcSlice = rpcTempCU->getPic()->getSlice(0);
    Bool bSliceEnd = (pcSlice->getSliceCurEndCUAddr() > rpcTempCU->getSCUAddr() && pcSlice->getSliceCurEndCUAddr() < rpcTempCU->getSCUAddr() + rpcTempCU->getTotalNumPart());
    Bool bInsidePicture = (uiRPelX < rpcTempCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < rpcTempCU->getSlice()->getSPS()->getPicHeightInLumaSamples());
    // We need to split, so don't try these modes.
    TComYuv* YuvTemp = NULL;
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit    = true;
    
        /*Initialise all Mode-CUs based on parentCU*/
        if(uiDepth==0)
        {
            m_InterCU_2Nx2N[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_InterCU_Nx2N[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_InterCU_2NxN[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_IntrainInterCU[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
            m_MergeCU[uiDepth]->initCU(pcCU->getPic(), pcCU->getAddr());
        }
        else
        {
            m_InterCU_2Nx2N[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP); 
            m_InterCU_2NxN[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP); 
            m_InterCU_Nx2N[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP); 
            m_IntrainInterCU[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP); 
            m_MergeCU[uiDepth]->initSubCU(pcCU, PartitionIndex, uiDepth, iQP); 
        }

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
        rpcBestCU = m_InterCU_2Nx2N[uiDepth];
        
        YuvTemp = m_ppcPredYuvMode[0][uiDepth];
        m_ppcPredYuvMode[0][uiDepth] = m_ppcPredYuvBest[uiDepth];
        m_ppcPredYuvBest[uiDepth] = YuvTemp;

        if(m_InterCU_Nx2N[uiDepth]->getTotalCost() < rpcBestCU->getTotalCost())
        {
            rpcBestCU = m_InterCU_Nx2N[uiDepth];
            
            YuvTemp = m_ppcPredYuvMode[1][uiDepth];
            m_ppcPredYuvMode[1][uiDepth] = m_ppcPredYuvBest[uiDepth];
            m_ppcPredYuvBest[uiDepth] = YuvTemp;
        }
        if(m_InterCU_2NxN[uiDepth]->getTotalCost() < rpcBestCU->getTotalCost())
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

    rpcTempCU->initEstData(uiDepth, iQP);
    // further split    
    if (bSubBranch && bTrySplitDQP && uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth)
    {
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
                rpcTempCU->copyPartFrom(pcSubBestPartCU, uiPartUnitIdx, uhNextDepth); // Keep best part data to current temporary data.
                xCopyYuv2Tmp(pcSubBestPartCU->getTotalNumPart() * uiPartUnitIdx, uhNextDepth);                
            }
            else if (bInSlice)
            {
                pcSubTempPartCU->copyToPic(uhNextDepth);
                rpcTempCU->copyPartFrom(pcSubTempPartCU, uiPartUnitIdx, uhNextDepth);        
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
        if(rpcBestCU)
        {
            if(rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost())
            {
                rpcBestCU = rpcTempCU;
                            
                YuvTemp = m_ppcRecoYuvTemp[uiDepth];
                m_ppcRecoYuvTemp[uiDepth] = m_ppcRecoYuvBest[uiDepth];
                m_ppcRecoYuvBest[uiDepth] = YuvTemp;
            }
        }else
        {
            rpcBestCU = rpcTempCU;
            YuvTemp = m_ppcRecoYuvTemp[uiDepth];
            m_ppcRecoYuvTemp[uiDepth] = m_ppcRecoYuvBest[uiDepth];
            m_ppcRecoYuvBest[uiDepth] = YuvTemp;
        }
    }

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