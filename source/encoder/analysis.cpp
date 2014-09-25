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
* For more information, contact us at license @ x265.com.
*****************************************************************************/

#include "analysis.h"
#include "primitives.h"
#include "common.h"
#include "rdcost.h"
#include "encoder.h"
#include "PPA/ppa.h"

using namespace x265;

namespace {
// TO DO: Remove this function with a table.
int getDepthScanIdx(int x, int y, int size)
{
    if (size == 1)
        return 0;

    int depth = 0;
    int h = size >> 1;

    if (x >= h)
    {
        x -= h;
        depth += h * h;
    }

    if (y >= h)
    {
        y -= h;
        depth += 2 * h * h;
    }

    return depth + getDepthScanIdx(x, y, h);
}
}

Analysis::Analysis()
{
    m_bestPredYuv     = NULL;
    m_bestResiYuv     = NULL;
    m_bestRecoYuv     = NULL;

    m_tmpPredYuv      = NULL;
    m_tmpResiYuv      = NULL;
    m_tmpRecoYuv      = NULL;
    m_bestMergeRecoYuv = NULL;
    m_origYuv         = NULL;
    for (int i = 0; i < MAX_PRED_TYPES; i++)
        m_modePredYuv[i] = NULL;
}

bool Analysis::create(uint32_t numCUDepth, uint32_t maxWidth)
{
    X265_CHECK(numCUDepth <= NUM_CU_DEPTH, "invalid numCUDepth\n");

    m_bestPredYuv = new TComYuv*[numCUDepth];
    m_bestResiYuv = new ShortYuv*[numCUDepth];
    m_bestRecoYuv = new TComYuv*[numCUDepth];

    m_tmpPredYuv     = new TComYuv*[numCUDepth];
    m_modePredYuv[0] = new TComYuv*[numCUDepth];
    m_modePredYuv[1] = new TComYuv*[numCUDepth];
    m_modePredYuv[2] = new TComYuv*[numCUDepth];
    m_modePredYuv[3] = new TComYuv*[numCUDepth];
    m_modePredYuv[4] = new TComYuv*[numCUDepth];
    m_modePredYuv[5] = new TComYuv*[numCUDepth];

    m_tmpResiYuv = new ShortYuv*[numCUDepth];
    m_tmpRecoYuv = new TComYuv*[numCUDepth];

    m_bestMergeRecoYuv = new TComYuv*[numCUDepth];

    m_origYuv = new TComYuv*[numCUDepth];

    int csp       = m_param->internalCsp;
    bool tqBypass = m_param->bCULossless || m_param->bLossless;

    m_memPool = new TComDataCU[numCUDepth];

    bool ok = true;
    for (uint32_t i = 0; i < numCUDepth; i++)
    {
        uint32_t numPartitions = 1 << (g_maxFullDepth - i) * 2;
        uint32_t cuSize = maxWidth >> i;

        uint32_t sizeL = cuSize * cuSize;
        uint32_t sizeC = sizeL >> (CHROMA_H_SHIFT(csp) + CHROMA_V_SHIFT(csp));

        ok &= m_memPool[i].initialize(numPartitions, sizeL, sizeC, 8, tqBypass);

        m_interCU_2Nx2N[i]  = new TComDataCU;
        m_interCU_2Nx2N[i]->create(&m_memPool[i], numPartitions, cuSize, csp, 0, tqBypass);

        m_interCU_2NxN[i]   = new TComDataCU;
        m_interCU_2NxN[i]->create(&m_memPool[i], numPartitions, cuSize, csp, 1, tqBypass);

        m_interCU_Nx2N[i]   = new TComDataCU;
        m_interCU_Nx2N[i]->create(&m_memPool[i], numPartitions, cuSize, csp, 2, tqBypass);

        m_intraInInterCU[i] = new TComDataCU;
        m_intraInInterCU[i]->create(&m_memPool[i], numPartitions, cuSize, csp, 3, tqBypass);

        m_mergeCU[i]        = new TComDataCU;
        m_mergeCU[i]->create(&m_memPool[i], numPartitions, cuSize, csp, 4, tqBypass);

        m_bestMergeCU[i]    = new TComDataCU;
        m_bestMergeCU[i]->create(&m_memPool[i], numPartitions, cuSize, csp, 5, tqBypass);

        m_bestCU[i]         = new TComDataCU;
        m_bestCU[i]->create(&m_memPool[i], numPartitions, cuSize, csp, 6, tqBypass);

        m_tempCU[i]         = new TComDataCU;
        m_tempCU[i]->create(&m_memPool[i], numPartitions, cuSize, csp, 7, tqBypass);

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

void Analysis::destroy()
{
    uint32_t numCUDepth = g_maxCUDepth + 1;
    for (uint32_t i = 0; i < numCUDepth; i++)
    {
        m_memPool[i].destroy();

        delete m_interCU_2Nx2N[i];
        delete m_interCU_2NxN[i];
        delete m_interCU_Nx2N[i];
        delete m_intraInInterCU[i];
        delete m_mergeCU[i];
        delete m_bestMergeCU[i];
        delete m_bestCU[i];
        delete m_tempCU[i];

        if (m_bestPredYuv && m_bestPredYuv[i])
        {
            m_bestPredYuv[i]->destroy();
            delete m_bestPredYuv[i];
        }
        if (m_bestResiYuv && m_bestResiYuv[i])
        {
            m_bestResiYuv[i]->destroy();
            delete m_bestResiYuv[i];
        }
        if (m_bestRecoYuv && m_bestRecoYuv[i])
        {
            m_bestRecoYuv[i]->destroy();
            delete m_bestRecoYuv[i];
        }

        if (m_tmpPredYuv && m_tmpPredYuv[i])
        {
            m_tmpPredYuv[i]->destroy();
            delete m_tmpPredYuv[i];
        }
        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            if (m_modePredYuv[j] && m_modePredYuv[j][i])
            {
                m_modePredYuv[j][i]->destroy();
                delete m_modePredYuv[j][i];
            }
        }

        if (m_tmpResiYuv && m_tmpResiYuv[i])
        {
            m_tmpResiYuv[i]->destroy();
            delete m_tmpResiYuv[i];
        }
        if (m_tmpRecoYuv && m_tmpRecoYuv[i])
        {
            m_tmpRecoYuv[i]->destroy();
            delete m_tmpRecoYuv[i];
        }
        if (m_bestMergeRecoYuv && m_bestMergeRecoYuv[i])
        {
            m_bestMergeRecoYuv[i]->destroy();
            delete m_bestMergeRecoYuv[i];
        }

        if (m_origYuv && m_origYuv[i])
        {
            m_origYuv[i]->destroy();
            delete m_origYuv[i];
        }
    }

    delete [] m_memPool;
    delete [] m_bestPredYuv;
    delete [] m_bestResiYuv;
    delete [] m_bestRecoYuv;
    delete [] m_bestMergeRecoYuv;
    delete [] m_tmpPredYuv;

    for (int i = 0; i < MAX_PRED_TYPES; i++)
        delete [] m_modePredYuv[i];

    delete [] m_tmpResiYuv;
    delete [] m_tmpRecoYuv;
    delete [] m_origYuv;
}

#define EARLY_EXIT                  1
#define TOPSKIP                     1

void Analysis::loadCTUData(TComDataCU* parentCU)
{
    uint8_t cuRange[2]= {MIN_LOG2_CU_SIZE, g_log2Size[m_param->maxCUSize]};

    // Initialize the coding blocks inside the CTB
    for (int rangeIdx = cuRange[1], rangeCUIdx = 0; rangeIdx >= cuRange[0]; rangeIdx--)
    {
        uint32_t log2CUSize = rangeIdx;
        int32_t  blockSize  = 1 << log2CUSize;
        uint32_t b8Width    = 1 << (cuRange[1] - 3);
        uint32_t sbWidth    = 1 << (cuRange[1] - rangeIdx);
        int32_t last_level_flag = rangeIdx == cuRange[0];
        for (uint32_t sb_y = 0; sb_y < sbWidth; sb_y++)
        {
            for (uint32_t sb_x = 0; sb_x < sbWidth; sb_x++)
            {
                uint32_t depth_idx = getDepthScanIdx(sb_x, sb_y, sbWidth);
                uint32_t cuIdx = rangeCUIdx + depth_idx;
                uint32_t child_idx = rangeCUIdx + sbWidth * sbWidth + (depth_idx << 2);
                int32_t px = parentCU->getCUPelX() + sb_x * blockSize;
                int32_t py = parentCU->getCUPelY() + sb_y * blockSize;
                int32_t present_flag = px < parentCU->m_pic->m_origPicYuv->m_picWidth && py < parentCU->m_pic->m_origPicYuv->m_picHeight;
                int32_t split_mandatory_flag = present_flag && !last_level_flag && (px + blockSize > parentCU->m_pic->m_origPicYuv->m_picWidth || py + blockSize > parentCU->m_pic->m_origPicYuv->m_picHeight);

                CU *cu = parentCU->m_CULocalData + cuIdx;
                cu->log2CUSize = log2CUSize;
                cu->childIdx = child_idx;
                cu->offset[0] = sb_x * blockSize;
                cu->offset[1] = sb_y * blockSize;
                cu->encodeIdx = getDepthScanIdx(cu->offset[0] >> 3, cu->offset[1] >> 3, b8Width);
                cu->flags = 0;

                CU_SET_FLAG(cu->flags, CU::PRESENT, present_flag);
                CU_SET_FLAG(cu->flags, CU::SPLIT_MANDATORY | CU::SPLIT, split_mandatory_flag);
                CU_SET_FLAG(cu->flags, CU::LEAF, last_level_flag);
            }
        }
        rangeCUIdx += sbWidth * sbWidth;
    }
}

void Analysis::compressCU(TComDataCU* cu)
{
    Frame* pic = cu->m_pic;
    uint32_t cuAddr = cu->getAddr();

    if (cu->m_slice->m_pps->bUseDQP)
        m_bEncodeDQP = true;

    // initialize CU data
    m_bestCU[0]->initCU(pic, cuAddr);
    m_tempCU[0]->initCU(pic, cuAddr);

    // analysis of CU
    uint32_t numPartition = cu->getTotalNumPart();
    if (m_bestCU[0]->m_slice->m_sliceType == I_SLICE)
    {
        if (m_param->analysisMode == X265_ANALYSIS_LOAD && pic->m_intraData)
        {
            uint32_t zOrder = 0;
            compressSharedIntraCTU(m_bestCU[0], m_tempCU[0], false, cu->m_CULocalData, 
                &pic->m_intraData->depth[cuAddr * cu->m_numPartitions],
                &pic->m_intraData->partSizes[cuAddr * cu->m_numPartitions],
                &pic->m_intraData->modes[cuAddr * cu->m_numPartitions], zOrder);
        }
        else
        {
            compressIntraCU(m_bestCU[0], m_tempCU[0], false, cu->m_CULocalData);
            if (m_param->analysisMode == X265_ANALYSIS_SAVE && pic->m_intraData)
            {
                memcpy(&pic->m_intraData->depth[cuAddr * cu->m_numPartitions], m_bestCU[0]->getDepth(), sizeof(uint8_t) * cu->getTotalNumPart());
                memcpy(&pic->m_intraData->modes[cuAddr * cu->m_numPartitions], m_bestCU[0]->getLumaIntraDir(), sizeof(uint8_t) * cu->getTotalNumPart());
                memcpy(&pic->m_intraData->partSizes[cuAddr * cu->m_numPartitions], m_bestCU[0]->getPartitionSize(), sizeof(char) * cu->getTotalNumPart());
                pic->m_intraData->cuAddr[cuAddr] = cuAddr;
                pic->m_intraData->poc[cuAddr]    = cu->m_pic->m_POC;
            }
        }
        if (m_param->bLogCuStats || m_param->rc.bStatWrite)
        {
            uint32_t i = 0;
            do
            {
                m_log->totalCu++;
                uint32_t depth = cu->getDepth(i);
                int next = numPartition >> (depth * 2);
                m_log->qTreeIntraCnt[depth]++;
                if (depth == g_maxCUDepth && cu->getPartitionSize(i) != SIZE_2Nx2N)
                    m_log->cntIntraNxN++;
                else
                {
                    m_log->cntIntra[depth]++;
                    if (cu->getLumaIntraDir(i) > 1)
                        m_log->cuIntraDistribution[depth][ANGULAR_MODE_ID]++;
                    else
                        m_log->cuIntraDistribution[depth][cu->getLumaIntraDir(i)]++;
                }
                i += next;
            }
            while (i < numPartition);
        }
    }
    else
    {
        if (m_param->rdLevel < 5)
        {
            TComDataCU* outBestCU = NULL;

            /* At the start of analysis, the best CU is a null pointer
             * On return, it points to the CU encode with best chosen mode */
            compressInterCU_rd0_4(outBestCU, m_tempCU[0], cu, 0, cu->m_CULocalData, false, 0, 4);
        }
        else
            compressInterCU_rd5_6(m_bestCU[0], m_tempCU[0], 0, cu->m_CULocalData);

        if (m_param->bLogCuStats || m_param->rc.bStatWrite)
        {
            uint32_t i = 0;
            do
            {
                uint32_t depth = cu->getDepth(i);
                m_log->cntTotalCu[depth]++;
                int next = numPartition >> (depth * 2);
                if (cu->isSkipped(i))
                {
                    m_log->cntSkipCu[depth]++;
                    m_log->qTreeSkipCnt[depth]++;
                }
                else
                {
                    m_log->totalCu++;
                    if (cu->getPredictionMode(0) == MODE_INTER)
                    {
                        m_log->cntInter[depth]++;
                        m_log->qTreeInterCnt[depth]++;
                        if (cu->getPartitionSize(0) < AMP_ID)
                            m_log->cuInterDistribution[depth][cu->getPartitionSize(0)]++;
                        else
                            m_log->cuInterDistribution[depth][AMP_ID]++;
                    }
                    else if (cu->getPredictionMode(0) == MODE_INTRA)
                    {
                        m_log->qTreeIntraCnt[depth]++;
                        if (depth == g_maxCUDepth && cu->getPartitionSize(0) == SIZE_NxN)
                        {
                            m_log->cntIntraNxN++;
                        }
                        else
                        {
                            m_log->cntIntra[depth]++;
                            if (cu->getLumaIntraDir(0) > 1)
                                m_log->cuIntraDistribution[depth][ANGULAR_MODE_ID]++;
                            else
                                m_log->cuIntraDistribution[depth][cu->getLumaIntraDir(0)]++;
                        }
                    }
                }
                i = i + next;
            }
            while (i < numPartition);
        }
    }
}

void Analysis::compressIntraCU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, CU *cu)
{
    //PPAScopeEvent(CompressIntraCU + depth);
    Frame* pic = outBestCU->m_pic;
    uint32_t cuAddr = outBestCU->getAddr();
    uint32_t absPartIdx = outBestCU->getZorderIdxInCU();

    if (depth == 0)
        // get original YUV data from picture
        m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), cuAddr, absPartIdx);
    else
        // copy partition YUV from depth 0 CTU cache
        m_origYuv[0]->copyPartToYuv(m_origYuv[depth], absPartIdx);
    Slice* slice = outTempCU->m_slice;
    // We need to split, so don't try these modes.
    int cu_split_flag = !(cu->flags & CU::LEAF);
    int cu_unsplit_flag = !(cu->flags & CU::SPLIT_MANDATORY);

    if (cu_unsplit_flag)
    {
        m_quant.setQPforQuant(outTempCU);
        checkIntra(outTempCU, SIZE_2Nx2N, cu, NULL);
        checkBestMode(outBestCU, outTempCU, depth);

        if (depth == g_maxCUDepth)
        {
            checkIntra(outTempCU, SIZE_NxN, cu, NULL);
            checkBestMode(outBestCU, outTempCU, depth);
        }
        else
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->codeSplitFlag(outBestCU, 0, depth);
            outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        }
        if (m_rdCost.m_psyRd)
            outBestCU->m_totalPsyCost = m_rdCost.calcPsyRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits, outBestCU->m_psyEnergy);
        else
            outBestCU->m_totalRDCost  = m_rdCost.calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);

        // copy original YUV samples in lossless mode
        if (outBestCU->isLosslessCoded(0))
            fillOrigYUVBuffer(outBestCU, m_origYuv[depth]);
    }

    // further split
    if (cu_split_flag)
    {
        uint32_t    nextDepth     = depth + 1;
        TComDataCU* subBestPartCU = m_bestCU[nextDepth];
        TComDataCU* subTempPartCU = m_tempCU[nextDepth];
        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
        {
            CU *child_cu = pic->getCU(cuAddr)->m_CULocalData + cu->childIdx + partUnitIdx;
            int qp = outTempCU->getQP(0);
            subBestPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.
            if (child_cu->flags & CU::PRESENT)
            {
                subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.
                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                    m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[depth][CI_CURR_BEST]);
                else
                    m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[nextDepth][CI_NEXT_BEST]);

                compressIntraCU(subBestPartCU, subTempPartCU, nextDepth, child_cu);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth); // Keep best part data to current temporary data.
                m_bestRecoYuv[nextDepth]->copyToPartYuv(m_tmpRecoYuv[depth], subBestPartCU->getTotalNumPart() * partUnitIdx);
            }
            else
            {
                subBestPartCU->copyToPic(nextDepth);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth);
            }
        }
        if (cu_unsplit_flag)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->codeSplitFlag(outTempCU, 0, depth);
            outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        }

        if (m_rdCost.m_psyRd)
            outTempCU->m_totalPsyCost = m_rdCost.calcPsyRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits, outTempCU->m_psyEnergy);
        else
            outTempCU->m_totalRDCost = m_rdCost.calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        if (depth == slice->m_pps->maxCuDQPDepth && slice->m_pps->bUseDQP)
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
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
        }

        m_rdEntropyCoders[nextDepth][CI_NEXT_BEST].store(m_rdEntropyCoders[depth][CI_TEMP_BEST]);
        checkBestMode(outBestCU, outTempCU, depth); // RD compare current CU against split
    }

    // TODO: write the best CTU at the end of complete CTU analysis
    outBestCU->copyToPic(depth); // Copy Best data to Picture for next partition prediction.
    if (!cu_unsplit_flag)
        return;
    // Copy Yuv data to picture Yuv
    m_bestRecoYuv[depth]->copyToPicYuv(pic->getPicYuvRec(), cuAddr, absPartIdx);

#if CHECKED_BUILD || _DEBUG
    X265_CHECK(outBestCU->getPartitionSize(0) != SIZE_NONE, "no best partition size\n");
    X265_CHECK(outBestCU->getPredictionMode(0) != MODE_NONE, "no best partition mode\n");
    if (m_rdCost.m_psyRd)
    {
        X265_CHECK(outBestCU->m_totalPsyCost != MAX_INT64, "no best partition cost\n");
    }
    else
    {
        X265_CHECK(outBestCU->m_totalRDCost != MAX_INT64, "no best partition cost\n");
    }
#endif
}

void Analysis::compressSharedIntraCTU(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, CU *cu, uint8_t* sharedDepth,
                                      char* sharedPartSizes, uint8_t* sharedModes, uint32_t &zOrder)
{
    Frame* pic = outBestCU->m_pic;

    // if current depth == shared depth then skip further splitting.
    bool bSubBranch = true;

    // index to g_depthInc array to increment zOrder offset to next depth
    int32_t ctuToDepthIndex = g_maxCUDepth - 1;

    if (depth)
        m_origYuv[0]->copyPartToYuv(m_origYuv[depth], outBestCU->getZorderIdxInCU());
    else
        m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU());

    Slice* slice = outTempCU->m_slice;
    int32_t cu_split_flag = !(cu->flags & CU::LEAF);
    int32_t cu_unsplit_flag = !(cu->flags & CU::SPLIT_MANDATORY);

    if (cu_unsplit_flag && ((zOrder == outBestCU->getZorderIdxInCU()) && (depth == sharedDepth[zOrder])))
    {
        m_quant.setQPforQuant(outTempCU);
        checkIntra(outTempCU, (PartSize)sharedPartSizes[zOrder], cu, &sharedModes[zOrder]);
        checkBestMode(outBestCU, outTempCU, depth);

        if (!(depth == g_maxCUDepth))
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->codeSplitFlag(outBestCU, 0, depth);
            outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits();
        }

        // set current best CU cost to 0 marking as best CU present in shared CU data
        outBestCU->m_totalRDCost = 0;
        bSubBranch = false;

        // increment zOrder offset to point to next best depth in sharedDepth buffer
        zOrder += g_depthInc[ctuToDepthIndex][sharedDepth[zOrder]];
    }

    // copy original YUV samples in lossless mode
    if (outBestCU->isLosslessCoded(0))
        fillOrigYUVBuffer(outBestCU, m_origYuv[depth]);

    // further split
    if (cu_split_flag && bSubBranch)
    {
        uint32_t    nextDepth     = depth + 1;
        TComDataCU* subBestPartCU = m_bestCU[nextDepth];
        TComDataCU* subTempPartCU = m_tempCU[nextDepth];
        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
        {
            CU *child_cu = pic->getCU(outTempCU->getAddr())->m_CULocalData + cu->childIdx + partUnitIdx;
            int qp = outTempCU->getQP(0);
            subBestPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.
            if (child_cu->flags & CU::PRESENT)
            {
                subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.

                if (partUnitIdx) // initialize RD with previous depth buffer
                    m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[nextDepth][CI_NEXT_BEST]);
                else
                    m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[depth][CI_CURR_BEST]);

                // set current best CU cost to 1 marking as non-best CU by default
                subTempPartCU->m_totalRDCost = 1;

                compressSharedIntraCTU(subBestPartCU, subTempPartCU, nextDepth, child_cu, sharedDepth, sharedPartSizes, sharedModes, zOrder);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth); // Keep best part data to current temporary data.

                if (!subBestPartCU->m_totalRDCost) // if cost is 0, CU is best CU
                    outTempCU->m_totalRDCost = 0;  // set outTempCU cost to 0, so later check will use this CU as best CU

                m_bestRecoYuv[nextDepth]->copyToPartYuv(m_tmpRecoYuv[depth], subBestPartCU->getTotalNumPart() * partUnitIdx);
            }
            else
            {
                subBestPartCU->copyToPic(nextDepth);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth);

                // increment zOrder offset to point to next best depth in sharedDepth buffer
                zOrder += g_depthInc[ctuToDepthIndex][nextDepth];
            }
        }

        if (cu_unsplit_flag)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->codeSplitFlag(outTempCU, 0, depth);
            outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        }
        if (depth == slice->m_pps->maxCuDQPDepth && slice->m_pps->bUseDQP)
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
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
        }
        m_rdEntropyCoders[nextDepth][CI_NEXT_BEST].store(m_rdEntropyCoders[depth][CI_TEMP_BEST]);
        checkBestMode(outBestCU, outTempCU, depth);
    }
    outBestCU->copyToPic(depth);
    if (!cu_unsplit_flag)
        return;
    m_bestRecoYuv[depth]->copyToPicYuv(pic->getPicYuvRec(), outBestCU->getAddr(), outBestCU->getZorderIdxInCU());

#if CHECKED_BUILD || _DEBUG
    X265_CHECK(outBestCU->getPartitionSize(0) != SIZE_NONE, "no best partition size\n");
    X265_CHECK(outBestCU->getPredictionMode(0) != MODE_NONE, "no best partition mode\n");
    if (m_rdCost.m_psyRd)
    {
        X265_CHECK(outBestCU->m_totalPsyCost != MAX_INT64, "no best partition cost\n");
    }
    else
    {
        X265_CHECK(outBestCU->m_totalRDCost != MAX_INT64, "no best partition cost\n");
    }
#endif
}

void Analysis::checkIntra(TComDataCU*& outTempCU, PartSize partSize, CU *cu, uint8_t* sharedModes)
{
    //PPAScopeEvent(CheckRDCostIntra + depth);
    uint32_t depth = g_log2Size[m_param->maxCUSize] - cu->log2CUSize;
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);

    uint32_t tuDepthRange[2];
    outTempCU->getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    if (sharedModes)
        sharedEstIntraPredQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], tuDepthRange, sharedModes);
    else
        estIntraPredQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], tuDepthRange);

    estIntraPredChromaQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);

    m_entropyCoder->resetBits();
    if (outTempCU->m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder->codeCUTransquantBypassFlag(outTempCU->getCUTransquantBypass(0));

    m_entropyCoder->codePartSize(outTempCU, 0, depth);
    m_entropyCoder->codePredInfo(outTempCU, 0);
    outTempCU->m_mvBits = m_entropyCoder->getNumberOfWrittenBits();

    // Encode Coefficients
    bool bCodeDQP = m_bEncodeDQP;
    m_entropyCoder->codeCoeff(outTempCU, 0, depth, bCodeDQP, tuDepthRange);
    m_entropyCoder->store(m_rdEntropyCoders[depth][CI_TEMP_BEST]);
    outTempCU->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();
    outTempCU->m_coeffBits = outTempCU->m_totalBits - outTempCU->m_mvBits;

    /* TODO: add chroma psyEnergy also to psyCost*/
    if (m_rdCost.m_psyRd)
    {
        int part = outTempCU->getLog2CUSize(0) - 2;
        outTempCU->m_psyEnergy = m_rdCost.psyCost(part, m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                  m_tmpRecoYuv[depth]->getLumaAddr(), m_tmpRecoYuv[depth]->getStride());
        outTempCU->m_totalPsyCost = m_rdCost.calcPsyRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits, outTempCU->m_psyEnergy);
    }
    else
        outTempCU->m_totalRDCost = m_rdCost.calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

    checkDQP(outTempCU);
}

void Analysis::compressInterCU_rd0_4(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComDataCU* parentCU, uint32_t depth, CU *cu, 
                                     int bInsidePicture, uint32_t PartitionIndex, uint32_t minDepth)
{
    Frame* pic = outTempCU->m_pic;
    uint32_t cuAddr = outTempCU->getAddr();
    uint32_t absPartIdx = outTempCU->getZorderIdxInCU();

    if (depth == 0)
        // get original YUV data from picture
        m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), cuAddr, absPartIdx);
    else
        // copy partition YUV from depth 0 CTU cache
        m_origYuv[0]->copyPartToYuv(m_origYuv[depth], absPartIdx);

    // variables for fast encoder decision
    bool bSubBranch = true;
    int qp = outTempCU->getQP(0);

#if TOPSKIP
    int bInsidePictureParent = bInsidePicture;
#endif

    Slice* slice = outTempCU->m_slice;
    int cu_split_flag = !(cu->flags & CU::LEAF);
    int cu_unsplit_flag = !(cu->flags & CU::SPLIT_MANDATORY);

    if (depth == 0 && m_param->rdLevel == 0)
    {
        m_origYuv[depth]->copyToPicYuv(pic->getPicYuvRec(), cuAddr, 0);
    }
    // We need to split, so don't try these modes.
#if TOPSKIP
    if (cu_unsplit_flag && !bInsidePictureParent)
    {
        TComDataCU* colocated0 = slice->m_numRefIdx[0] > 0 ? slice->m_refPicList[0][0]->getCU(cuAddr) : NULL;
        TComDataCU* colocated1 = slice->m_numRefIdx[1] > 0 ? slice->m_refPicList[1][0]->getCU(cuAddr) : NULL;
        char currentQP = outTempCU->getQP(0);
        char previousQP = colocated0->getQP(0);
        uint32_t delta = 0, minDepth0 = 4, minDepth1 = 4;
        uint32_t sum0 = 0, sum1 = 0;
        uint32_t numPartitions = outTempCU->getTotalNumPart();
        for (uint32_t i = 0; i < numPartitions; i = i + 4)
        {
            uint32_t j = absPartIdx + i;
            if (colocated0 && colocated0->getDepth(j) < minDepth0)
                minDepth0 = colocated0->getDepth(j);
            if (colocated1 && colocated1->getDepth(j) < minDepth1)
                minDepth1 = colocated1->getDepth(j);
            if (colocated0)
                sum0 += (colocated0->getDepth(j) * 4);
            if (colocated1)
                sum1 += (colocated1->getDepth(j) * 4);
        }

        uint32_t avgDepth2 = (sum0 + sum1) / numPartitions;
        minDepth = X265_MIN(minDepth0, minDepth1);
        if (((currentQP - previousQP) < 0) || (((currentQP - previousQP) >= 0) && ((avgDepth2 - 2 * minDepth) > 1)))
            delta = 0;
        else
            delta = 1;
        if (minDepth > 0)
            minDepth = minDepth - delta;
    }
    if (!(depth < minDepth)) //topskip
#endif // if TOPSKIP
    {
        if (cu_unsplit_flag)
        {
            /* Initialise all Mode-CUs based on parentCU */
            if (depth == 0)
            {
                m_interCU_2Nx2N[depth]->initCU(pic, cuAddr);
                m_interCU_Nx2N[depth]->initCU(pic, cuAddr);
                m_interCU_2NxN[depth]->initCU(pic, cuAddr);
                m_intraInInterCU[depth]->initCU(pic, cuAddr);
                m_mergeCU[depth]->initCU(pic, cuAddr);
                m_bestMergeCU[depth]->initCU(pic, cuAddr);
            }
            else
            {
                m_interCU_2Nx2N[depth]->initSubCU(parentCU, PartitionIndex, depth, qp);
                m_interCU_2NxN[depth]->initSubCU(parentCU, PartitionIndex, depth, qp);
                m_interCU_Nx2N[depth]->initSubCU(parentCU, PartitionIndex, depth, qp);
                m_intraInInterCU[depth]->initSubCU(parentCU, PartitionIndex, depth, qp);
                m_mergeCU[depth]->initSubCU(parentCU, PartitionIndex, depth, qp);
                m_bestMergeCU[depth]->initSubCU(parentCU, PartitionIndex, depth, qp);
            }

            /* Compute  Merge Cost */
            checkMerge2Nx2N_rd0_4(m_bestMergeCU[depth], m_mergeCU[depth], m_modePredYuv[3][depth], m_bestMergeRecoYuv[depth]);
            bool earlyskip = false;
            if (m_param->rdLevel >= 1)
                earlyskip = (m_param->bEnableEarlySkip && m_bestMergeCU[depth]->isSkipped(0));

            if (!earlyskip)
            {
                /* Compute 2Nx2N mode costs */
                {
                    checkInter_rd0_4(m_interCU_2Nx2N[depth], m_modePredYuv[0][depth], SIZE_2Nx2N);
                    /* Choose best mode; initialise outBestCU to 2Nx2N */
                    outBestCU = m_interCU_2Nx2N[depth];
                    std::swap(m_bestPredYuv[depth], m_modePredYuv[0][depth]);
                }

                /* Compute Rect costs */
                if (m_param->bEnableRectInter)
                {
                    checkInter_rd0_4(m_interCU_Nx2N[depth], m_modePredYuv[1][depth], SIZE_Nx2N);
                    checkInter_rd0_4(m_interCU_2NxN[depth], m_modePredYuv[2][depth], SIZE_2NxN);
                    if (m_interCU_Nx2N[depth]->m_sa8dCost < outBestCU->m_sa8dCost)
                    {
                        outBestCU = m_interCU_Nx2N[depth];
                        std::swap(m_bestPredYuv[depth], m_modePredYuv[1][depth]);
                    }
                    if (m_interCU_2NxN[depth]->m_sa8dCost < outBestCU->m_sa8dCost)
                    {
                        outBestCU = m_interCU_2NxN[depth];
                        std::swap(m_bestPredYuv[depth], m_modePredYuv[2][depth]);
                    }
                }

                if (m_param->rdLevel > 2)
                {
                    // calculate the motion compensation for chroma for the best mode selected
                    int numPart = outBestCU->getNumPartInter();
                    for (int partIdx = 0; partIdx < numPart; partIdx++)
                    {
                        prepMotionCompensation(outBestCU, partIdx);
                        motionCompensation(m_bestPredYuv[depth], false, true);
                    }

                    encodeResAndCalcRdInterCU(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth],
                                              m_bestResiYuv[depth], m_bestRecoYuv[depth]);
                    uint64_t bestMergeCost = m_rdCost.m_psyRd ? m_bestMergeCU[depth]->m_totalPsyCost : m_bestMergeCU[depth]->m_totalRDCost;
                    uint64_t bestCost = m_rdCost.m_psyRd ? outBestCU->m_totalPsyCost : outBestCU->m_totalRDCost;
                    if (bestMergeCost < bestCost)
                    {
                        outBestCU = m_bestMergeCU[depth];
                        std::swap(m_bestPredYuv[depth], m_modePredYuv[3][depth]);
                        std::swap(m_bestRecoYuv[depth], m_bestMergeRecoYuv[depth]);
                    }
                    else
                    {
                        m_rdEntropyCoders[depth][CI_TEMP_BEST].store(m_rdEntropyCoders[depth][CI_NEXT_BEST]);
                    }
                }

                /* Check for Intra in inter frames only if its a P-slice*/
                if (slice->m_sliceType == P_SLICE)
                {
                    /* compute intra cost */
                    bool bdoIntra = true;

                    if (m_param->rdLevel > 2)
                    {
                        bdoIntra = (outBestCU->getCbf(0, TEXT_LUMA) ||  outBestCU->getCbf(0, TEXT_CHROMA_U) ||
                                    outBestCU->getCbf(0, TEXT_CHROMA_V));
                    }
                    if (bdoIntra)
                    {
                        checkIntraInInter_rd0_4(m_intraInInterCU[depth], SIZE_2Nx2N);
                        uint64_t intraInInterCost, bestCost;
                        if (m_param->rdLevel > 2)
                        {
                            encodeIntraInInter(m_intraInInterCU[depth], m_origYuv[depth], m_modePredYuv[5][depth],
                                               m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);
                            intraInInterCost = m_rdCost.m_psyRd ? m_intraInInterCU[depth]->m_totalPsyCost : m_intraInInterCU[depth]->m_totalRDCost;
                            bestCost = m_rdCost.m_psyRd ? outBestCU->m_totalPsyCost : outBestCU->m_totalRDCost;
                        }
                        else
                        {
                            intraInInterCost = m_intraInInterCU[depth]->m_sa8dCost;
                            bestCost = outBestCU->m_sa8dCost;
                        }
                        if (intraInInterCost < bestCost)
                        {
                            outBestCU = m_intraInInterCU[depth];
                            std::swap(m_bestPredYuv[depth], m_modePredYuv[5][depth]);
                            std::swap(m_bestRecoYuv[depth], m_tmpRecoYuv[depth]);
                            if (m_param->rdLevel > 2)
                                m_rdEntropyCoders[depth][CI_TEMP_BEST].store(m_rdEntropyCoders[depth][CI_NEXT_BEST]);
                        }
                    }
                }
                if (m_param->rdLevel == 2)
                {
                    if (m_bestMergeCU[depth]->m_sa8dCost < outBestCU->m_sa8dCost)
                    {
                        outBestCU = m_bestMergeCU[depth];
                        std::swap(m_bestPredYuv[depth], m_modePredYuv[3][depth]);
                        std::swap(m_bestRecoYuv[depth], m_bestMergeRecoYuv[depth]);
                    }
                    else if (outBestCU->getPredictionMode(0) == MODE_INTER)
                    {
                        int numPart = outBestCU->getNumPartInter();
                        for (int partIdx = 0; partIdx < numPart; partIdx++)
                        {
                            prepMotionCompensation(outBestCU, partIdx);
                            motionCompensation(m_bestPredYuv[depth], false, true);
                        }

                        encodeResAndCalcRdInterCU(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth],
                                                  m_bestResiYuv[depth], m_bestRecoYuv[depth]);
                        m_rdEntropyCoders[depth][CI_TEMP_BEST].store(m_rdEntropyCoders[depth][CI_NEXT_BEST]);
                    }
                    else if (outBestCU->getPredictionMode(0) == MODE_INTRA)
                    {
                        encodeIntraInInter(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth],  m_bestRecoYuv[depth]);
                        m_rdEntropyCoders[depth][CI_TEMP_BEST].store(m_rdEntropyCoders[depth][CI_NEXT_BEST]);
                    }
                }
                else if (m_param->rdLevel == 1)
                {
                    if (m_bestMergeCU[depth]->m_sa8dCost < outBestCU->m_sa8dCost)
                    {
                        outBestCU = m_bestMergeCU[depth];
                        std::swap(m_bestPredYuv[depth], m_modePredYuv[3][depth]);
                        std::swap(m_bestRecoYuv[depth], m_bestMergeRecoYuv[depth]);
                    }
                    else if (outBestCU->getPredictionMode(0) == MODE_INTER)
                    {
                        int numPart = outBestCU->getNumPartInter();
                        for (int partIdx = 0; partIdx < numPart; partIdx++)
                        {
                            prepMotionCompensation(outBestCU, partIdx);
                            motionCompensation(m_bestPredYuv[depth], false, true);
                        }

                        m_tmpResiYuv[depth]->subtract(m_origYuv[depth], m_bestPredYuv[depth], outBestCU->getLog2CUSize(0));
                        generateCoeffRecon(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth], m_bestRecoYuv[depth]);
                    }
                    else
                        generateCoeffRecon(outBestCU, m_origYuv[depth], m_bestPredYuv[depth], m_tmpResiYuv[depth], m_bestRecoYuv[depth]);
                }
                else if (m_param->rdLevel == 0)
                {
                    if (outBestCU->getPredictionMode(0) == MODE_INTER)
                    {
                        int numPart = outBestCU->getNumPartInter();
                        for (int partIdx = 0; partIdx < numPart; partIdx++)
                        {
                            prepMotionCompensation(outBestCU, partIdx);
                            motionCompensation(m_bestPredYuv[depth], false, true);
                        }
                    }
                }
            }
            else
            {
                outBestCU = m_bestMergeCU[depth];
                std::swap(m_bestPredYuv[depth], m_modePredYuv[3][depth]);
                std::swap(m_bestRecoYuv[depth], m_bestMergeRecoYuv[depth]);
            }

            if (m_param->rdLevel > 0) // checkDQP can be done only after residual encoding is done
                checkDQP(outBestCU);
            /* Disable recursive analysis for whole CUs temporarily */
            if ((outBestCU != 0) && (outBestCU->isSkipped(0)))
                bSubBranch = false;
            else
                bSubBranch = true;

            if (m_param->rdLevel > 1)
            {
                if (depth < g_maxCUDepth)
                {
                    m_entropyCoder->resetBits();
                    m_entropyCoder->codeSplitFlag(outBestCU, 0, depth);
                    outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
                }
                if (m_rdCost.m_psyRd)
                    outBestCU->m_totalPsyCost = m_rdCost.calcPsyRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits, outBestCU->m_psyEnergy);
                else
                    outBestCU->m_totalRDCost = m_rdCost.calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);
            }

            // copy original YUV samples in lossless mode
            if (outBestCU->isLosslessCoded(0))
                fillOrigYUVBuffer(outBestCU, m_origYuv[depth]);
        }
    }

    // further split
    if (bSubBranch && cu_split_flag)
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
            uint64_t totalCostNeigh = 0, totalCostCU = 0, totalCountNeigh = 0, totalCountCU = 0;
            TComDataCU* above = outTempCU->getCUAbove();
            TComDataCU* aboveLeft = outTempCU->getCUAboveLeft();
            TComDataCU* aboveRight = outTempCU->getCUAboveRight();
            TComDataCU* left = outTempCU->getCULeft();
            TComDataCU* rootCU = pic->getPicSym()->getCU(cuAddr);

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

            // give 60% weight to all CU's and 40% weight to neighbour CU's
            uint64_t avgCost = 0;
            if (totalCountNeigh + totalCountCU)
                avgCost = ((3 * totalCostCU) + (2 * totalCostNeigh)) / ((3 * totalCountCU) + (2 * totalCountNeigh));
            uint64_t bestavgCost = 0;
            if (m_param->rdLevel > 1)
                bestavgCost = m_rdCost.m_psyRd ? outBestCU->m_totalPsyCost : outBestCU->m_totalRDCost;
            else
                bestavgCost = outBestCU->m_totalRDCost;

            if (bestavgCost < avgCost && avgCost != 0 && depth != 0)
            {
                /* Copy Best data to Picture for next partition prediction. */
                outBestCU->copyToPic(depth);

                /* Copy Yuv data to picture Yuv */
                if (m_param->rdLevel != 0)
                    m_bestRecoYuv[depth]->copyToPicYuv(pic->getPicYuvRec(), cuAddr, absPartIdx);
                return;
            }
        }
#endif // if EARLY_EXIT
        outTempCU->setQPSubParts(qp, 0, depth);
        uint32_t    nextDepth = depth + 1;
        TComDataCU* subTempPartCU = m_tempCU[nextDepth];
        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
        {
            CU *child_cu = pic->getCU(cuAddr)->m_CULocalData + cu->childIdx + partUnitIdx;

            TComDataCU* subBestPartCU = NULL;
            subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.

            if (child_cu->flags & CU::PRESENT)
            {
                if (0 == partUnitIdx) // initialize RD with previous depth buffer
                    m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[depth][CI_CURR_BEST]);
                else
                    m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[nextDepth][CI_NEXT_BEST]);

                compressInterCU_rd0_4(subBestPartCU, subTempPartCU, outTempCU, nextDepth, child_cu, cu_unsplit_flag, partUnitIdx, minDepth);
#if EARLY_EXIT
                if (subBestPartCU->getPredictionMode(0) != MODE_INTRA)
                {
                    uint64_t tempavgCost = 0;
                    if (m_param->rdLevel > 1)
                        tempavgCost = m_rdCost.m_psyRd ? subBestPartCU->m_totalPsyCost : subBestPartCU->m_totalRDCost;
                    else
                        tempavgCost = subBestPartCU->m_totalRDCost;
                    TComDataCU* rootCU = pic->getPicSym()->getCU(cuAddr);
                    uint64_t temp = rootCU->m_avgCost[nextDepth] * rootCU->m_count[nextDepth];
                    rootCU->m_count[nextDepth] += 1;
                    rootCU->m_avgCost[nextDepth] = (temp + tempavgCost) / rootCU->m_count[nextDepth];
                }
#endif // if EARLY_EXIT
                /* Adding costs from best SUbCUs */
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth, true); // Keep best part data to current temporary data.
                if (m_param->rdLevel != 0)
                    m_bestRecoYuv[nextDepth]->copyToPartYuv(m_tmpRecoYuv[depth], subBestPartCU->getTotalNumPart() * partUnitIdx);
                else
                    m_bestPredYuv[nextDepth]->copyToPartYuv(m_tmpPredYuv[depth], subBestPartCU->getTotalNumPart() * partUnitIdx);
            }
            else
            {
                subTempPartCU->copyToPic(nextDepth);
                outTempCU->copyPartFrom(subTempPartCU, partUnitIdx, nextDepth, false);
            }
        }

        if (cu_unsplit_flag)
        {
            if (m_param->rdLevel > 1)
            {
                m_entropyCoder->resetBits();
                m_entropyCoder->codeSplitFlag(outTempCU, 0, depth);
                outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
            }
        }
        if (m_param->rdLevel > 1)
        {
            if (m_rdCost.m_psyRd)
                outTempCU->m_totalPsyCost = m_rdCost.calcPsyRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits, outTempCU->m_psyEnergy);
            else
                outTempCU->m_totalRDCost = m_rdCost.calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);
        }
        else
            outTempCU->m_sa8dCost = m_rdCost.calcRdSADCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        if (depth == slice->m_pps->maxCuDQPDepth && slice->m_pps->bUseDQP)
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
                X265_CHECK(foundNonZeroCbf, "setQPSubCUs did not find non-zero Cbf\n");
            }
            else
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
        }

        m_rdEntropyCoders[nextDepth][CI_NEXT_BEST].store(m_rdEntropyCoders[depth][CI_TEMP_BEST]);

        /* If Best Mode is not NULL; then compare costs. Else assign best mode to Sub-CU costs
         * Copy recon data from Temp structure to Best structure */
        if (outBestCU)
        {
#if EARLY_EXIT
            if (depth == 0)
            {
                uint64_t tempavgCost = m_rdCost.m_psyRd ? outBestCU->m_totalPsyCost : outBestCU->m_totalRDCost;
                TComDataCU* rootCU = pic->getPicSym()->getCU(cuAddr);
                uint64_t temp = rootCU->m_avgCost[depth] * rootCU->m_count[depth];
                rootCU->m_count[depth] += 1;
                rootCU->m_avgCost[depth] = (temp + tempavgCost) / rootCU->m_count[depth];
            }
#endif
            uint64_t tempCost = m_rdCost.m_psyRd ? outTempCU->m_totalPsyCost : outTempCU->m_totalRDCost;
            uint64_t bestCost = m_rdCost.m_psyRd ? outBestCU->m_totalPsyCost : outBestCU->m_totalRDCost; 
            if (tempCost < bestCost)
            {
                outBestCU = outTempCU;
                std::swap(m_bestRecoYuv[depth], m_tmpRecoYuv[depth]);
                std::swap(m_bestPredYuv[depth], m_tmpPredYuv[depth]);
            }
        }
        else
        {
            outBestCU = outTempCU;
            std::swap(m_bestRecoYuv[depth], m_tmpRecoYuv[depth]);
            std::swap(m_bestPredYuv[depth], m_tmpPredYuv[depth]);
        }
    }

    /* Copy Best data to Picture for next partition prediction. */
    outBestCU->copyToPic(depth);

    if (m_param->rdLevel == 0 && depth == 0)
        encodeResidue(outBestCU, outBestCU, 0, 0);
    else if (m_param->rdLevel != 0)
    {
        /* Copy Yuv data to picture Yuv */
        if (cu_unsplit_flag)
            m_bestRecoYuv[depth]->copyToPicYuv(pic->getPicYuvRec(), cuAddr, absPartIdx);
    }

#if CHECKED_BUILD || _DEBUG
    /* Assert if Best prediction mode is NONE
     * Selected mode's RD-cost must be not MAX_INT64 */
    if (cu_unsplit_flag)
    {
        X265_CHECK(outBestCU->getPartitionSize(0) != SIZE_NONE, "no best prediction size\n");
        X265_CHECK(outBestCU->getPredictionMode(0) != MODE_NONE, "no best prediction mode\n");
        if (m_param->rdLevel > 1)
        {
            if (m_rdCost.m_psyRd)
            {
                X265_CHECK(outBestCU->m_totalPsyCost != MAX_INT64, "no best partition cost\n");
            }
            else
            {
                X265_CHECK(outBestCU->m_totalRDCost != MAX_INT64, "no best partition cost\n");
            }
        }
        else
        {
            X265_CHECK(outBestCU->m_sa8dCost != MAX_INT64, "no best partition cost\n");
        }
    }
#endif

    x265_emms();
}

void Analysis::compressInterCU_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth, CU *cu, PartSize parentSize)
{
    //PPAScopeEvent(CompressCU + depth);

    Frame* pic = outBestCU->m_pic;
    uint32_t cuAddr = outBestCU->getAddr();
    uint32_t absPartIdx = outBestCU->getZorderIdxInCU();

    if (depth == 0)
        // get original YUV data from picture
        m_origYuv[depth]->copyFromPicYuv(pic->getPicYuvOrg(), cuAddr, absPartIdx);
    else
        // copy partition YUV from depth 0 CTU cache
        m_origYuv[0]->copyPartToYuv(m_origYuv[depth], absPartIdx);

    // variable for Cbf fast mode PU decision
    bool doNotBlockPu = true;
    bool earlyDetectionSkipMode = false;

    Slice* slice = outTempCU->m_slice;
    int cu_split_flag = !(cu->flags & CU::LEAF);
    int cu_unsplit_flag = !(cu->flags & CU::SPLIT_MANDATORY);

    // We need to split, so don't try these modes.
    if (cu_unsplit_flag)
    {
        m_quant.setQPforQuant(outTempCU);

        // do inter modes, SKIP and 2Nx2N
        if (slice->m_sliceType != I_SLICE)
        {
            // by Merge for inter_2Nx2N
            checkMerge2Nx2N_rd5_6(outBestCU, outTempCU, &earlyDetectionSkipMode, m_bestPredYuv[depth], m_bestRecoYuv[depth]);

            outTempCU->initEstData();

            if (!m_param->bEnableEarlySkip)
            {
                // 2Nx2N, NxN
                checkInter_rd5_6(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData();
                if (m_param->bEnableCbfFastMode)
                    doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
            }
        }

        if (!earlyDetectionSkipMode)
        {
            outTempCU->initEstData();

            // do inter modes, NxN, 2NxN, and Nx2N
            if (slice->m_sliceType != I_SLICE)
            {
                // 2Nx2N, NxN
                if (!(cu->log2CUSize == 3))
                {
                    if (depth == g_maxCUDepth && doNotBlockPu)
                    {
                        checkInter_rd5_6(outBestCU, outTempCU, SIZE_NxN);
                        outTempCU->initEstData();
                    }
                }

                if (m_param->bEnableRectInter)
                {
                    // 2NxN, Nx2N
                    if (doNotBlockPu)
                    {
                        checkInter_rd5_6(outBestCU, outTempCU, SIZE_Nx2N);
                        outTempCU->initEstData();
                        if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_Nx2N)
                            doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                    }
                    if (doNotBlockPu)
                    {
                        checkInter_rd5_6(outBestCU, outTempCU, SIZE_2NxN);
                        outTempCU->initEstData();
                        if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxN)
                            doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                    }
                }

                // Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
                if (slice->m_sps->maxAMPDepth > depth)
                {
                    bool bTestAMP_Hor = false, bTestAMP_Ver = false;
                    bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

                    deriveTestModeAMP(outBestCU, parentSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);

                    // Do horizontal AMP
                    if (bTestAMP_Hor)
                    {
                        if (doNotBlockPu)
                        {
                            checkInter_rd5_6(outBestCU, outTempCU, SIZE_2NxnU);
                            outTempCU->initEstData();
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                        if (doNotBlockPu)
                        {
                            checkInter_rd5_6(outBestCU, outTempCU, SIZE_2NxnD);
                            outTempCU->initEstData();
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                    }
                    else if (bTestMergeAMP_Hor)
                    {
                        if (doNotBlockPu)
                        {
                            checkInter_rd5_6(outBestCU, outTempCU, SIZE_2NxnU, true);
                            outTempCU->initEstData();
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnU)
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                        if (doNotBlockPu)
                        {
                            checkInter_rd5_6(outBestCU, outTempCU, SIZE_2NxnD, true);
                            outTempCU->initEstData();
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_2NxnD)
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                    }

                    // Do horizontal AMP
                    if (bTestAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            checkInter_rd5_6(outBestCU, outTempCU, SIZE_nLx2N);
                            outTempCU->initEstData();
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                        if (doNotBlockPu)
                        {
                            checkInter_rd5_6(outBestCU, outTempCU, SIZE_nRx2N);
                            outTempCU->initEstData();
                        }
                    }
                    else if (bTestMergeAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            checkInter_rd5_6(outBestCU, outTempCU, SIZE_nLx2N, true);
                            outTempCU->initEstData();
                            if (m_param->bEnableCbfFastMode && outBestCU->getPartitionSize(0) == SIZE_nLx2N)
                                doNotBlockPu = outBestCU->getQtRootCbf(0) != 0;
                        }
                        if (doNotBlockPu)
                        {
                            checkInter_rd5_6(outBestCU, outTempCU, SIZE_nRx2N, true);
                            outTempCU->initEstData();
                        }
                    }
                }
            }

            // speedup for inter frames, avoid intra in special cases
            bool doIntra = slice->m_sliceType == B_SLICE ? !!m_param->bIntraInBFrames : true;
            if ((outBestCU->getCbf(0, TEXT_LUMA)     != 0   ||
                 outBestCU->getCbf(0, TEXT_CHROMA_U) != 0   ||
                 outBestCU->getCbf(0, TEXT_CHROMA_V) != 0)  && doIntra)
            {
                checkIntraInInter_rd5_6(outBestCU, outTempCU, SIZE_2Nx2N);
                outTempCU->initEstData();

                if (depth == g_maxCUDepth)
                {
                    if (cu->log2CUSize > slice->m_sps->quadtreeTULog2MinSize)
                    {
                        checkIntraInInter_rd5_6(outBestCU, outTempCU, SIZE_NxN);
                        outTempCU->initEstData();
                    }
                }
            }
        }

        if (depth < g_maxCUDepth)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->codeSplitFlag(outBestCU, 0, depth);
            outBestCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        }
        if (m_rdCost.m_psyRd)
            outBestCU->m_totalPsyCost = m_rdCost.calcPsyRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits, outBestCU->m_psyEnergy);
        else
            outBestCU->m_totalRDCost = m_rdCost.calcRdCost(outBestCU->m_totalDistortion, outBestCU->m_totalBits);

        // copy original YUV samples in lossless mode
        if (outBestCU->isLosslessCoded(0))
            fillOrigYUVBuffer(outBestCU, m_origYuv[depth]);
    }

    // further split
    if (cu_split_flag && !outBestCU->isSkipped(0))
    {
        uint32_t    nextDepth     = depth + 1;
        TComDataCU* subBestPartCU = m_bestCU[nextDepth];
        TComDataCU* subTempPartCU = m_tempCU[nextDepth];
        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
        {
            CU *child_cu = pic->getCU(cuAddr)->m_CULocalData + cu->childIdx + partUnitIdx;

            int qp = outTempCU->getQP(0);
            subBestPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.

            if (child_cu->flags & CU::PRESENT)
            {
                subTempPartCU->initSubCU(outTempCU, partUnitIdx, nextDepth, qp); // clear sub partition datas or init.

                if (0 == partUnitIdx) //initialize RD with previous depth buffer
                    m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[depth][CI_CURR_BEST]);
                else
                    m_rdEntropyCoders[nextDepth][CI_CURR_BEST].load(m_rdEntropyCoders[nextDepth][CI_NEXT_BEST]);

                compressInterCU_rd5_6(subBestPartCU, subTempPartCU, nextDepth, child_cu);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth); // Keep best part data to current temporary data.
                m_bestRecoYuv[nextDepth]->copyToPartYuv(m_tmpRecoYuv[depth], subBestPartCU->getTotalNumPart() * partUnitIdx);
            }
            else
            {
                subBestPartCU->copyToPic(nextDepth);
                outTempCU->copyPartFrom(subBestPartCU, partUnitIdx, nextDepth);
            }
        }

        if (cu_unsplit_flag)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->codeSplitFlag(outTempCU, 0, depth);
            outTempCU->m_totalBits += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        }

        if (m_rdCost.m_psyRd)
            outTempCU->m_totalPsyCost = m_rdCost.calcPsyRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits, outTempCU->m_psyEnergy);
        else
            outTempCU->m_totalRDCost = m_rdCost.calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

        if (depth == slice->m_pps->maxCuDQPDepth && slice->m_pps->bUseDQP)
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
                outTempCU->setQPSubParts(outTempCU->getRefQP(targetPartIdx), 0, depth); // set QP to default QP
        }

        m_rdEntropyCoders[nextDepth][CI_NEXT_BEST].store(m_rdEntropyCoders[depth][CI_TEMP_BEST]);
        checkBestMode(outBestCU, outTempCU, depth); // RD compare current CU against split
    }
    outBestCU->copyToPic(depth); // Copy Best data to Picture for next partition prediction.

    // Copy Yuv data to picture Yuv
    m_bestRecoYuv[depth]->copyToPicYuv(pic->getPicYuvRec(), cuAddr, absPartIdx);

#if CHECKED_BUILD || _DEBUG
    X265_CHECK(outBestCU->getPartitionSize(0) != SIZE_NONE, "no best partition size\n");
    X265_CHECK(outBestCU->getPredictionMode(0) != MODE_NONE, "no best partition mode\n");
    if (m_rdCost.m_psyRd)
    {
        X265_CHECK(outBestCU->m_totalPsyCost != MAX_INT64, "no best partition cost\n");
    }
    else
    {
        X265_CHECK(outBestCU->m_totalRDCost != MAX_INT64, "no best partition cost\n");
    }
#endif
}

void Analysis::checkMerge2Nx2N_rd0_4(TComDataCU*& outBestCU, TComDataCU*& outTempCU, TComYuv*& bestPredYuv, TComYuv*& yuvReconBest)
{
    X265_CHECK(outTempCU->m_slice->m_sliceType != I_SLICE, "Evaluating merge in I slice\n");
    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
    uint8_t interDirNeighbours[MRG_MAX_NUM_CANDS];
    uint32_t maxNumMergeCand = outTempCU->m_slice->m_maxNumMergeCand;

    uint32_t depth = outTempCU->getDepth(0);
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
    outTempCU->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);
    outTempCU->getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours, maxNumMergeCand);
    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outTempCU->setMergeFlag(0, true);

    outBestCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
    outBestCU->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);
    outBestCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outBestCU->setMergeFlag(0, true);

    int sizeIdx = outTempCU->getLog2CUSize(0) - 2;
    int bestMergeCand = -1;

    for (uint32_t mergeCand = 0; mergeCand < maxNumMergeCand; ++mergeCand)
    {
        if (!m_bFrameParallel ||
            (mvFieldNeighbours[mergeCand][0].mv.y < (m_param->searchRange + 1) * 4 &&
             mvFieldNeighbours[mergeCand][1].mv.y < (m_param->searchRange + 1) * 4))
        {
            // set MC parameters, interprets depth relative to LCU level
            outTempCU->setMergeIndex(0, mergeCand);
            outTempCU->setInterDirSubParts(interDirNeighbours[mergeCand], 0, 0, depth);
            outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[mergeCand][0], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level
            outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[mergeCand][1], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level

            // do MC only for Luma part
            /* Set CU parameters for motion compensation */
            prepMotionCompensation(outTempCU, 0);
            motionCompensation(m_tmpPredYuv[depth], true, false);
            uint32_t bitsCand = getTUBits(mergeCand, maxNumMergeCand);
            outTempCU->m_totalBits = bitsCand;
            outTempCU->m_totalDistortion = primitives.sa8d[sizeIdx](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                                    m_tmpPredYuv[depth]->getLumaAddr(), m_tmpPredYuv[depth]->getStride());
            outTempCU->m_sa8dCost = m_rdCost.calcRdSADCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

            if (outTempCU->m_sa8dCost < outBestCU->m_sa8dCost)
            {
                bestMergeCand = mergeCand;
                std::swap(outBestCU, outTempCU);
                // Change Prediction data
                std::swap(bestPredYuv, m_tmpPredYuv[depth]);
            }
        }
    }

    if (bestMergeCand < 0)
    {
        outBestCU->setMergeFlag(0, false);
        outBestCU->setQPSubParts(outBestCU->getQP(0), 0, depth);
    }
    else
    {
        outTempCU->setMergeIndex(0, bestMergeCand);
        outTempCU->setInterDirSubParts(interDirNeighbours[bestMergeCand], 0, 0, depth);
        outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[bestMergeCand][0], SIZE_2Nx2N, 0, 0);
        outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[bestMergeCand][1], SIZE_2Nx2N, 0, 0);
        outTempCU->m_totalBits = outBestCU->m_totalBits;
        outTempCU->m_totalDistortion = outBestCU->m_totalDistortion;
        outTempCU->m_sa8dCost = outBestCU->m_sa8dCost;

        if (m_param->rdLevel >= 1)
        {
            // calculate the motion compensation for chroma for the best mode selected
            int numPart = outBestCU->getNumPartInter();
            for (int partIdx = 0; partIdx < numPart; partIdx++)
            {
                prepMotionCompensation(outBestCU, partIdx);
                motionCompensation(bestPredYuv, false, true);
            }

            if (outTempCU->isLosslessCoded(0))
                outBestCU->m_totalRDCost = MAX_INT64;
            else
            {
                // No-residue mode
                encodeResAndCalcRdSkipCU(outBestCU, m_origYuv[depth], bestPredYuv, m_tmpRecoYuv[depth]);
                std::swap(yuvReconBest, m_tmpRecoYuv[depth]);
                m_rdEntropyCoders[depth][CI_TEMP_BEST].store(m_rdEntropyCoders[depth][CI_NEXT_BEST]);
            }

            // Encode with residue
            encodeResAndCalcRdInterCU(outTempCU, m_origYuv[depth], bestPredYuv, m_tmpResiYuv[depth], m_bestResiYuv[depth], m_tmpRecoYuv[depth]);

            uint64_t tempCost = m_rdCost.m_psyRd ? outTempCU->m_totalPsyCost : outTempCU->m_totalRDCost;
            uint64_t bestCost = m_rdCost.m_psyRd ? outBestCU->m_totalPsyCost : outBestCU->m_totalRDCost;
            if (tempCost < bestCost) // Choose best from no-residue mode and residue mode
            {
                std::swap(outBestCU, outTempCU);
                std::swap(yuvReconBest, m_tmpRecoYuv[depth]);
                m_rdEntropyCoders[depth][CI_TEMP_BEST].store(m_rdEntropyCoders[depth][CI_NEXT_BEST]);
            }
        }
    }
}

void Analysis::checkMerge2Nx2N_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, bool *earlyDetectionSkipMode, TComYuv*& outBestPredYuv, TComYuv*& rpcYuvReconBest)
{
    X265_CHECK(outTempCU->m_slice->m_sliceType != I_SLICE, "I slice not expected\n");
    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
    uint8_t interDirNeighbours[MRG_MAX_NUM_CANDS];
    uint32_t maxNumMergeCand = outTempCU->m_slice->m_maxNumMergeCand;

    uint32_t depth = outTempCU->getDepth(0);
    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
    outTempCU->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);
    outTempCU->getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours, maxNumMergeCand);

    int mergeCandBuffer[MRG_MAX_NUM_CANDS];
    for (uint32_t i = 0; i < maxNumMergeCand; ++i)
        mergeCandBuffer[i] = 0;

    bool bestIsSkip = false;

    uint32_t iteration = outTempCU->isLosslessCoded(0) ? 1 : 2;

    for (uint32_t noResidual = 0; noResidual < iteration; ++noResidual)
    {
        for (uint32_t mergeCand = 0; mergeCand < maxNumMergeCand; ++mergeCand)
        {
            if (m_bFrameParallel &&
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
                    outTempCU->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);
                    outTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to LCU level
                    outTempCU->setMergeFlag(0, true);
                    outTempCU->setMergeIndex(0, mergeCand);
                    outTempCU->setInterDirSubParts(interDirNeighbours[mergeCand], 0, 0, depth); // interprets depth relative to LCU level
                    outTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(mvFieldNeighbours[mergeCand][0], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level
                    outTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(mvFieldNeighbours[mergeCand][1], SIZE_2Nx2N, 0, 0); // interprets depth relative to outTempCU level

                    // do MC
                    prepMotionCompensation(outTempCU, 0);
                    motionCompensation(m_tmpPredYuv[depth], true, true);

                    // estimate residual and encode everything
                    if (noResidual)
                        encodeResAndCalcRdSkipCU(outTempCU,
                                                 m_origYuv[depth],
                                                 m_tmpPredYuv[depth],
                                                 m_tmpRecoYuv[depth]);
                    else
                        encodeResAndCalcRdInterCU(outTempCU,
                                                  m_origYuv[depth],
                                                  m_tmpPredYuv[depth],
                                                  m_tmpResiYuv[depth],
                                                  m_bestResiYuv[depth],
                                                  m_tmpRecoYuv[depth]);


                    /* Todo: Fix the satd cost estimates. Why is merge being chosen in high motion areas: estimated distortion is too low? */
                    if (!noResidual && !outTempCU->getQtRootCbf(0))
                        mergeCandBuffer[mergeCand] = 1;

                    outTempCU->setSkipFlagSubParts(!outTempCU->getQtRootCbf(0), 0, depth);
                    int origQP = outTempCU->getQP(0);
                    checkDQP(outTempCU);
                    uint64_t tempCost = m_rdCost.m_psyRd ? outTempCU->m_totalPsyCost : outTempCU->m_totalRDCost;
                    uint64_t bestCost = m_rdCost.m_psyRd ? outBestCU->m_totalPsyCost : outBestCU->m_totalRDCost;
                    if (tempCost < bestCost)
                    {
                        std::swap(outBestCU, outTempCU);
                        std::swap(outBestPredYuv, m_tmpPredYuv[depth]);
                        std::swap(rpcYuvReconBest, m_tmpRecoYuv[depth]);

                        m_rdEntropyCoders[depth][CI_TEMP_BEST].store(m_rdEntropyCoders[depth][CI_NEXT_BEST]);
                    }
                    outTempCU->setQPSubParts(origQP, 0, depth);
                    outTempCU->setSkipFlagSubParts(false, 0, depth);
                    if (!bestIsSkip)
                        bestIsSkip = !outBestCU->getQtRootCbf(0);
                }
            }
        }

        if (!noResidual && m_param->bEnableEarlySkip && !outBestCU->getQtRootCbf(0))
        {
            if (outBestCU->getMergeFlag(0))
                *earlyDetectionSkipMode = true;
            else
            {
                bool noMvd = true;
                for (uint32_t refListIdx = 0; refListIdx < 2; refListIdx++)
                    if (outBestCU->m_slice->m_numRefIdx[refListIdx] > 0)
                        noMvd &= !outBestCU->getCUMvField(refListIdx)->getMvd(0).word;
                if (noMvd)
                    *earlyDetectionSkipMode = true;
            }
        }
    }
}

void Analysis::checkInter_rd0_4(TComDataCU* outTempCU, TComYuv* outPredYuv, PartSize partSize, bool bUseMRG)
{
    uint32_t depth = outTempCU->getDepth(0);

    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);

    // do motion compensation only for Luma since luma cost alone is calculated
    outTempCU->m_totalBits = 0;
    if (predInterSearch(outTempCU, outPredYuv, bUseMRG, false))
    {
        int sizeIdx = outTempCU->getLog2CUSize(0) - 2;
        uint32_t distortion = primitives.sa8d[sizeIdx](m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                       outPredYuv->getLumaAddr(), outPredYuv->getStride());
        outTempCU->m_totalDistortion = distortion;
        outTempCU->m_sa8dCost = m_rdCost.calcRdSADCost(distortion, outTempCU->m_totalBits);
    }
    else
    {
        outTempCU->m_totalDistortion = MAX_UINT;
        outTempCU->m_totalRDCost = MAX_INT64;
    }
}

void Analysis::checkInter_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize, bool bUseMRG)
{
    uint32_t depth = outTempCU->getDepth(0);

    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTER, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);

    if (predInterSearch(outTempCU, m_tmpPredYuv[depth], bUseMRG, true))
    {
        encodeResAndCalcRdInterCU(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_bestResiYuv[depth], m_tmpRecoYuv[depth]);
        checkDQP(outTempCU);
        checkBestMode(outBestCU, outTempCU, depth);
    }
}

void Analysis::checkIntraInInter_rd0_4(TComDataCU* cu, PartSize partSize)
{
    uint32_t depth = cu->getDepth(0);

    cu->setPartSizeSubParts(partSize, 0, depth);
    cu->setPredModeSubParts(MODE_INTRA, 0, depth);
    cu->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);

    uint32_t initTrDepth = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
    uint32_t log2TrSize  = cu->getLog2CUSize(0) - initTrDepth;
    uint32_t tuSize      = 1 << log2TrSize;
    const uint32_t partOffset  = 0;

    // Reference sample smoothing
    TComPattern::initAdiPattern(cu, partOffset, initTrDepth, m_predBuf, m_refAbove, m_refLeft, m_refAboveFlt, m_refLeftFlt, ALL_IDX);

    pixel* fenc     = m_origYuv[depth]->getLumaAddr();
    uint32_t stride = m_modePredYuv[5][depth]->getStride();

    pixel *above         = m_refAbove    + tuSize - 1;
    pixel *aboveFiltered = m_refAboveFlt + tuSize - 1;
    pixel *left          = m_refLeft     + tuSize - 1;
    pixel *leftFiltered  = m_refLeftFlt  + tuSize - 1;
    int sad, bsad;
    uint32_t bits, bbits, mode, bmode;
    uint64_t cost, bcost;

    // 33 Angle modes once
    ALIGN_VAR_32(pixel, buf_trans[32 * 32]);
    ALIGN_VAR_32(pixel, tmp[33 * 32 * 32]);
    int scaleTuSize = tuSize;
    int scaleStride = stride;
    int costShift = 0;
    int sizeIdx = log2TrSize - 2;

    if (tuSize > 32)
    {
        // origin is 64x64, we scale to 32x32 and setup required parameters
        ALIGN_VAR_32(pixel, bufScale[32 * 32]);
        primitives.scale2D_64to32(bufScale, fenc, stride);
        fenc = bufScale;

        // reserve space in case primitives need to store data in above
        // or left buffers
        pixel _above[4 * 32 + 1];
        pixel _left[4 * 32 + 1];
        pixel *aboveScale  = _above + 2 * 32;
        pixel *leftScale   = _left + 2 * 32;
        aboveScale[0] = leftScale[0] = above[0];
        primitives.scale1D_128to64(aboveScale + 1, above + 1, 0);
        primitives.scale1D_128to64(leftScale + 1, left + 1, 0);

        scaleTuSize = 32;
        scaleStride = 32;
        costShift = 2;
        sizeIdx = 5 - 2; // log2(scaleTuSize) - 2

        // Filtered and Unfiltered refAbove and refLeft pointing to above and left.
        above         = aboveScale;
        left          = leftScale;
        aboveFiltered = aboveScale;
        leftFiltered  = leftScale;
    }

    pixelcmp_t sa8d = primitives.sa8d[sizeIdx];
    int predsize = scaleTuSize * scaleTuSize;

    uint32_t preds[3];
    cu->getIntraDirLumaPredictor(partOffset, preds);

    uint64_t mpms;
    uint32_t rbits = getIntraRemModeBits(cu, partOffset, depth, preds, mpms);

    // DC
    primitives.intra_pred[DC_IDX][sizeIdx](tmp, scaleStride, left, above, 0, (scaleTuSize <= 16));
    bsad = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;
    bmode = mode = DC_IDX;
    bbits = (mpms & ((uint64_t)1 << mode)) ? getIntraModeBits(cu, mode, partOffset, depth) : rbits;
    bcost = m_rdCost.calcRdSADCost(bsad, bbits);

    pixel *abovePlanar   = above;
    pixel *leftPlanar    = left;

    if (tuSize & (8 | 16 | 32))
    {
        abovePlanar = aboveFiltered;
        leftPlanar  = leftFiltered;
    }

    // PLANAR
    primitives.intra_pred[PLANAR_IDX][sizeIdx](tmp, scaleStride, leftPlanar, abovePlanar, 0, 0);
    sad = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;
    mode = PLANAR_IDX;
    bits = (mpms & ((uint64_t)1 << mode)) ? getIntraModeBits(cu, mode, partOffset, depth) : rbits;
    cost = m_rdCost.calcRdSADCost(sad, bits);
    COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);

    // Transpose NxN
    primitives.transpose[sizeIdx](buf_trans, fenc, scaleStride);

    primitives.intra_pred_allangs[sizeIdx](tmp, above, left, aboveFiltered, leftFiltered, (scaleTuSize <= 16));

    bool modeHor;
    pixel *cmp;
    intptr_t srcStride;

#define TRY_ANGLE(angle) \
    modeHor = angle < 18; \
    cmp = modeHor ? buf_trans : fenc; \
    srcStride = modeHor ? scaleTuSize : scaleStride; \
    sad = sa8d(cmp, srcStride, &tmp[(angle - 2) * predsize], scaleTuSize) << costShift; \
    bits = (mpms & ((uint64_t)1 << angle)) ? getIntraModeBits(cu, angle, partOffset, depth) : rbits; \
    cost = m_rdCost.calcRdSADCost(sad, bits)

    if (m_param->bEnableFastIntra)
    {
        int asad = 0;
        uint32_t lowmode, highmode, amode = 5, abits = 0;
        uint64_t acost = MAX_INT64;

        /* pick the best angle, sampling at distance of 5 */
        for (mode = 5; mode < 35; mode += 5)
        {
            TRY_ANGLE(mode);
            COPY4_IF_LT(acost, cost, amode, mode, asad, sad, abits, bits);
        }

        /* refine best angle at distance 2, then distance 1 */
        for (uint32_t dist = 2; dist >= 1; dist--)
        {
            lowmode = amode - dist;
            highmode = amode + dist;

            X265_CHECK(lowmode >= 2 && lowmode <= 34, "low intra mode out of range\n");
            TRY_ANGLE(lowmode);
            COPY4_IF_LT(acost, cost, amode, lowmode, asad, sad, abits, bits);

            X265_CHECK(highmode >= 2 && highmode <= 34, "high intra mode out of range\n");
            TRY_ANGLE(highmode);
            COPY4_IF_LT(acost, cost, amode, highmode, asad, sad, abits, bits);
        }

        if (amode == 33)
        {
            TRY_ANGLE(34);
            COPY4_IF_LT(acost, cost, amode, 34, asad, sad, abits, bits);
        }

        COPY4_IF_LT(bcost, acost, bmode, amode, bsad, asad, bbits, abits);
    }
    else // calculate and search all intra prediction angles for lowest cost
    {
        for (mode = 2; mode < 35; mode++)
        {
            TRY_ANGLE(mode);
            COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);
        }
    }
    cu->m_totalBits = bbits;
    cu->m_totalDistortion = bsad;
    cu->m_sa8dCost = bcost;

    // generate predYuv for the best mode
    cu->setLumaIntraDirSubParts(bmode, partOffset, depth + initTrDepth);
}

void Analysis::checkIntraInInter_rd5_6(TComDataCU*& outBestCU, TComDataCU*& outTempCU, PartSize partSize)
{
    uint32_t depth = outTempCU->getDepth(0);

    PPAScopeEvent(CheckRDCostIntra + depth);

    m_quant.setQPforQuant(outTempCU);
    outTempCU->setSkipFlagSubParts(false, 0, depth);
    outTempCU->setPartSizeSubParts(partSize, 0, depth);
    outTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    outTempCU->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);

    uint32_t tuDepthRange[2];
    outTempCU->getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    estIntraPredQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth], tuDepthRange);

    estIntraPredChromaQT(outTempCU, m_origYuv[depth], m_tmpPredYuv[depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);

    m_entropyCoder->resetBits();
    if (outTempCU->m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder->codeCUTransquantBypassFlag(outTempCU->getCUTransquantBypass(0));

    if (!outTempCU->m_slice->isIntra())
    {
        m_entropyCoder->codeSkipFlag(outTempCU, 0);
        m_entropyCoder->codePredMode(outTempCU->getPredictionMode(0));
    }
    m_entropyCoder->codePartSize(outTempCU, 0, depth);
    m_entropyCoder->codePredInfo(outTempCU, 0);
    outTempCU->m_mvBits = m_entropyCoder->getNumberOfWrittenBits();

    // Encode Coefficients
    bool bCodeDQP = m_bEncodeDQP;
    m_entropyCoder->codeCoeff(outTempCU, 0, depth, bCodeDQP, tuDepthRange);
    m_entropyCoder->store(m_rdEntropyCoders[depth][CI_TEMP_BEST]);
    outTempCU->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();
    outTempCU->m_coeffBits = outTempCU->m_totalBits - outTempCU->m_mvBits;

    if (m_rdCost.m_psyRd)
    {
        int part = outTempCU->getLog2CUSize(0) - 2;
        outTempCU->m_psyEnergy = m_rdCost.psyCost(part, m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
                                                  m_tmpRecoYuv[depth]->getLumaAddr(), m_tmpRecoYuv[depth]->getStride());
        outTempCU->m_totalPsyCost = m_rdCost.calcPsyRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits, outTempCU->m_psyEnergy);
    }
    else
        outTempCU->m_totalRDCost = m_rdCost.calcRdCost(outTempCU->m_totalDistortion, outTempCU->m_totalBits);

    checkDQP(outTempCU);
    checkBestMode(outBestCU, outTempCU, depth);
}

void Analysis::encodeIntraInInter(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv,  ShortYuv* outResiYuv, TComYuv* outReconYuv)
{
    uint64_t puCost = 0;
    uint32_t puBits = 0;
    uint32_t psyEnergy = 0;
    uint32_t depth = cu->getDepth(0);
    uint32_t initTrDepth = cu->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;

    // set context models
    m_entropyCoder->load(m_rdEntropyCoders[depth][CI_CURR_BEST]);

    m_quant.setQPforQuant(cu);

    uint32_t tuDepthRange[2];
    cu->getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    uint32_t puDistY = xRecurIntraCodingQT(cu, initTrDepth, 0, fencYuv, predYuv, outResiYuv, false, puCost, puBits, psyEnergy, tuDepthRange);
    xSetIntraResultQT(cu, initTrDepth, 0, outReconYuv);

    //=== update PU data ====
    cu->copyToPic(cu->getDepth(0), 0, initTrDepth);

    //===== set distortion (rate and r-d costs are determined later) =====
    cu->m_totalDistortion = puDistY;

    estIntraPredChromaQT(cu, fencYuv, predYuv, outResiYuv, outReconYuv);
    m_entropyCoder->resetBits();
    if (cu->m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder->codeCUTransquantBypassFlag(cu->getCUTransquantBypass(0));

    if (!cu->m_slice->isIntra())
    {
        m_entropyCoder->codeSkipFlag(cu, 0);
        m_entropyCoder->codePredMode(cu->getPredictionMode(0));
    }
    m_entropyCoder->codePartSize(cu, 0, depth);
    m_entropyCoder->codePredInfo(cu, 0);
    cu->m_mvBits += m_entropyCoder->getNumberOfWrittenBits();

    // Encode Coefficients
    bool bCodeDQP = m_bEncodeDQP;
    m_entropyCoder->codeCoeff(cu, 0, depth, bCodeDQP, tuDepthRange);
    m_entropyCoder->store(m_rdEntropyCoders[depth][CI_TEMP_BEST]);

    cu->m_totalBits = m_entropyCoder->getNumberOfWrittenBits();
    cu->m_coeffBits = cu->m_totalBits - cu->m_mvBits;
    if (m_rdCost.m_psyRd)
    {
        int part = cu->getLog2CUSize(0) - 2;
        cu->m_psyEnergy = m_rdCost.psyCost(part, m_origYuv[depth]->getLumaAddr(), m_origYuv[depth]->getStride(),
            m_tmpRecoYuv[depth]->getLumaAddr(), m_tmpRecoYuv[depth]->getStride());
        cu->m_totalPsyCost = m_rdCost.calcPsyRdCost(cu->m_totalDistortion, cu->m_totalBits, cu->m_psyEnergy);
    }
    else
        cu->m_totalRDCost = m_rdCost.calcRdCost(cu->m_totalDistortion, cu->m_totalBits);
}

void Analysis::encodeResidue(TComDataCU* lcu, TComDataCU* cu, uint32_t absPartIdx, uint32_t depth)
{
    Frame* pic = cu->m_pic;

    if (depth < lcu->getDepth(absPartIdx) && depth < g_maxCUDepth)
    {
        Slice* slice = cu->m_slice;
        uint32_t nextDepth = depth + 1;
        TComDataCU* subTempPartCU = m_tempCU[nextDepth];
        uint32_t qNumParts = (NUM_CU_PARTITIONS >> (depth << 1)) >> 2;
        uint32_t xmax = slice->m_sps->picWidthInLumaSamples  - lcu->getCUPelX();
        uint32_t ymax = slice->m_sps->picHeightInLumaSamples - lcu->getCUPelY();        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++, absPartIdx += qNumParts)
        {
            if (g_zscanToPelX[absPartIdx] < xmax && g_zscanToPelY[absPartIdx] < ymax)
            {
                subTempPartCU->copyToSubCU(cu, partUnitIdx, nextDepth);
                encodeResidue(lcu, subTempPartCU, absPartIdx, nextDepth);
            }
        }

        return;
    }

    uint32_t cuAddr = cu->getAddr();

    m_quant.setQPforQuant(cu);

    if (lcu->getPredictionMode(absPartIdx) == MODE_INTER)
    {
        int log2CUSize = cu->getLog2CUSize(0);
        if (!lcu->getSkipFlag(absPartIdx))
        {
            const int sizeIdx = log2CUSize - 2;
            // Calculate Residue
            pixel* src2 = m_bestPredYuv[0]->getLumaAddr(absPartIdx);
            pixel* src1 = m_origYuv[0]->getLumaAddr(absPartIdx);
            int16_t* dst = m_tmpResiYuv[depth]->getLumaAddr();
            uint32_t src2stride = m_bestPredYuv[0]->getStride();
            uint32_t src1stride = m_origYuv[0]->getStride();
            uint32_t dststride = m_tmpResiYuv[depth]->m_width;
            primitives.luma_sub_ps[sizeIdx](dst, dststride, src1, src2, src1stride, src2stride);

            src2 = m_bestPredYuv[0]->getCbAddr(absPartIdx);
            src1 = m_origYuv[0]->getCbAddr(absPartIdx);
            dst = m_tmpResiYuv[depth]->getCbAddr();
            src2stride = m_bestPredYuv[0]->getCStride();
            src1stride = m_origYuv[0]->getCStride();
            dststride = m_tmpResiYuv[depth]->m_cwidth;
            primitives.chroma[m_param->internalCsp].sub_ps[sizeIdx](dst, dststride, src1, src2, src1stride, src2stride);

            src2 = m_bestPredYuv[0]->getCrAddr(absPartIdx);
            src1 = m_origYuv[0]->getCrAddr(absPartIdx);
            dst = m_tmpResiYuv[depth]->getCrAddr();
            primitives.chroma[m_param->internalCsp].sub_ps[sizeIdx](dst, dststride, src1, src2, src1stride, src2stride);

            uint32_t tuDepthRange[2];
            cu->getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);
            // Residual encoding
            residualTransformQuantInter(cu, 0, m_origYuv[0], m_tmpResiYuv[depth], cu->getDepth(0), tuDepthRange);
            checkDQP(cu);

            if (lcu->getMergeFlag(absPartIdx) && cu->getPartitionSize(0) == SIZE_2Nx2N && !cu->getQtRootCbf(0))
            {
                cu->setSkipFlagSubParts(true, 0, depth);
                cu->copyCodedToPic(depth);
            }
            else
            {
                cu->copyCodedToPic(depth);

                // Generate Recon
                pixel* pred = m_bestPredYuv[0]->getLumaAddr(absPartIdx);
                int16_t* res = m_tmpResiYuv[depth]->getLumaAddr();
                pixel* reco = m_bestRecoYuv[depth]->getLumaAddr();
                dststride = m_bestRecoYuv[depth]->getStride();
                src1stride = m_bestPredYuv[0]->getStride();
                src2stride = m_tmpResiYuv[depth]->m_width;
                primitives.luma_add_ps[sizeIdx](reco, dststride, pred, res, src1stride, src2stride);

                pred = m_bestPredYuv[0]->getCbAddr(absPartIdx);
                res = m_tmpResiYuv[depth]->getCbAddr();
                reco = m_bestRecoYuv[depth]->getCbAddr();
                dststride = m_bestRecoYuv[depth]->getCStride();
                src1stride = m_bestPredYuv[0]->getCStride();
                src2stride = m_tmpResiYuv[depth]->m_cwidth;
                primitives.chroma[m_param->internalCsp].add_ps[sizeIdx](reco, dststride, pred, res, src1stride, src2stride);

                pred = m_bestPredYuv[0]->getCrAddr(absPartIdx);
                res = m_tmpResiYuv[depth]->getCrAddr();
                reco = m_bestRecoYuv[depth]->getCrAddr();
                primitives.chroma[m_param->internalCsp].add_ps[sizeIdx](reco, dststride, pred, res, src1stride, src2stride);
                m_bestRecoYuv[depth]->copyToPicYuv(pic->getPicYuvRec(), cuAddr, absPartIdx);
                return;
            }
        }

        // Generate Recon
        int part = partitionFromLog2Size(log2CUSize);
        TComPicYuv* rec = pic->getPicYuvRec();
        pixel* src = m_bestPredYuv[0]->getLumaAddr(absPartIdx);
        pixel* dst = rec->getLumaAddr(cuAddr, absPartIdx);
        uint32_t srcstride = m_bestPredYuv[0]->getStride();
        uint32_t dststride = rec->getStride();
        primitives.luma_copy_pp[part](dst, dststride, src, srcstride);

        src = m_bestPredYuv[0]->getCbAddr(absPartIdx);
        dst = rec->getCbAddr(cuAddr, absPartIdx);
        srcstride = m_bestPredYuv[0]->getCStride();
        dststride = rec->getCStride();
        primitives.chroma[m_param->internalCsp].copy_pp[part](dst, dststride, src, srcstride);

        src = m_bestPredYuv[0]->getCrAddr(absPartIdx);
        dst = rec->getCrAddr(cuAddr, absPartIdx);
        primitives.chroma[m_param->internalCsp].copy_pp[part](dst, dststride, src, srcstride);
    }
    else
    {
        m_origYuv[0]->copyPartToYuv(m_origYuv[depth], absPartIdx);
        generateCoeffRecon(cu, m_origYuv[depth], m_modePredYuv[5][depth], m_tmpResiYuv[depth], m_tmpRecoYuv[depth]);
        checkDQP(cu);
        m_tmpRecoYuv[depth]->copyToPicYuv(pic->getPicYuvRec(), cuAddr, absPartIdx);
        cu->copyCodedToPic(depth);
    }
}

/* check whether current try is the best with identifying the depth of current try */
void Analysis::checkBestMode(TComDataCU*& outBestCU, TComDataCU*& outTempCU, uint32_t depth)
{
    uint64_t tempCost = m_rdCost.m_psyRd ? outTempCU->m_totalPsyCost : outTempCU->m_totalRDCost;
    uint64_t bestCost = m_rdCost.m_psyRd ? outBestCU->m_totalPsyCost : outBestCU->m_totalRDCost;

    if (tempCost < bestCost)
    {
        // Change Information data
        std::swap(outBestCU, outTempCU);

        // Change Prediction data
        std::swap(m_bestPredYuv[depth], m_tmpPredYuv[depth]);

        // Change Reconstruction data
        std::swap(m_bestRecoYuv[depth], m_tmpRecoYuv[depth]);

        m_rdEntropyCoders[depth][CI_TEMP_BEST].store(m_rdEntropyCoders[depth][CI_NEXT_BEST]);
    }
}

void Analysis::deriveTestModeAMP(TComDataCU* outBestCU, PartSize parentSize, bool &bTestAMP_Hor, bool &bTestAMP_Ver,
                                 bool &bTestMergeAMP_Hor, bool &bTestMergeAMP_Ver)
{
    if (outBestCU->getPartitionSize(0) == SIZE_2NxN)
        bTestAMP_Hor = true;
    else if (outBestCU->getPartitionSize(0) == SIZE_Nx2N)
        bTestAMP_Ver = true;
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
            bTestMergeAMP_Hor = true;
        else if (outBestCU->getPartitionSize(0) == SIZE_Nx2N)
            bTestMergeAMP_Ver = true;
    }

    if (outBestCU->getPartitionSize(0) == SIZE_2Nx2N && outBestCU->isSkipped(0) == false)
    {
        bTestMergeAMP_Hor = true;
        bTestMergeAMP_Ver = true;
    }

    if (outBestCU->getLog2CUSize(0) == 6)
    {
        bTestAMP_Hor = false;
        bTestAMP_Ver = false;
    }
}

void Analysis::checkDQP(TComDataCU* cu)
{
    uint32_t depth = cu->getDepth(0);
    Slice* slice = cu->m_slice;

    if (slice->m_pps->bUseDQP && depth <= slice->m_pps->maxCuDQPDepth)
    {
        if (!cu->getCbf(0, TEXT_LUMA, 0) && !cu->getCbf(0, TEXT_CHROMA_U, 0) && !cu->getCbf(0, TEXT_CHROMA_V, 0))
            cu->setQPSubParts(cu->getRefQP(0), 0, depth); // set QP to default QP
    }
}

/* Function for filling original YUV samples of a CU in lossless mode */
void Analysis::fillOrigYUVBuffer(TComDataCU* cu, TComYuv* fencYuv)
{
    /* TODO: is this extra copy really necessary? the source pixels will still
     * be available when getLumaOrigYuv() is used */

    uint32_t width  = 1 << cu->getLog2CUSize(0);
    uint32_t height = 1 << cu->getLog2CUSize(0);

    pixel* srcY = fencYuv->getLumaAddr();
    pixel* dstY = cu->getLumaOrigYuv();
    uint32_t srcStride = fencYuv->getStride();

    /* TODO: square block copy primitive */
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
            dstY[x] = srcY[x];

        dstY += width;
        srcY += srcStride;
    }

    pixel* srcCb = fencYuv->getChromaAddr(1);
    pixel* srcCr = fencYuv->getChromaAddr(2);

    pixel* dstCb = cu->getChromaOrigYuv(1);
    pixel* dstCr = cu->getChromaOrigYuv(2);

    uint32_t srcStrideC = fencYuv->getCStride();
    uint32_t widthC  = width  >> cu->getHorzChromaShift();
    uint32_t heightC = height >> cu->getVertChromaShift();

    /* TODO: block copy primitives */
    for (uint32_t y = 0; y < heightC; y++)
    {
        for (uint32_t x = 0; x < widthC; x++)
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
